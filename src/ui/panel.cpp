#include "ui/panel.hpp"
#include "ui/dialogs/menus.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

// the `F` keys, which can be overriden by the nested views
static const std::array<int, 6> refreshable_keys = { VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8 };

panel::panel(spotify::api_abstract *api):
    api_proxy(api)
{
    ObserverManager::subscribe<ui_events_observer>(this);
}

panel::~panel()
{
    ObserverManager::unsubscribe<ui_events_observer>(this);
    view = nullptr;
}

void panel::update_panel_info(OpenPanelInfo *info)
{
    static wchar_t dir_name[64], title[64];
    
    const auto &view_cur_dir = view->get_dir_name();
    if (!view_cur_dir.empty())
    {
        // showing "Spotifar: Menu Item Name" title in case a cur dir is not empty
        config::fsf.snprintf(title, std::size(title), L" %s: %s ",
            far3::get_text(MPluginUserName), view_cur_dir.c_str());
    }
    else
    {
        // ...or just "Spotifar" plugin name
        config::fsf.snprintf(title, std::size(title), far3::get_text(MPluginUserName));
    }
    
    config::fsf.snprintf(dir_name, std::size(dir_name),
        utils::strip_invalid_filename_chars(view_cur_dir).c_str());

    info->StructSize = sizeof(*info);
    info->CurDir = dir_name;
    info->PanelTitle = title;
    info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;
    info->StartPanelMode = '3';
    info->StartSortMode = SM_NAME;

    // filling in the info lines on the Ctrl+L panel
    const auto &info_lines = view->get_info_lines();
    if (info_lines != nullptr)
    {
        info->InfoLines = info_lines->data();
        info->InfoLinesNumber = info_lines->size();
    }

    // every update we clear out all the refreshable `F` keys and fill them up
    // by demand with the overriding info from the nested view
    static KeyBarLabel key_bar_labels[refreshable_keys.size() * 4];
    static KeyBarTitles key_bar = { std::size(key_bar_labels), key_bar_labels };
    info->KeyBar = &key_bar;

    const auto view_key_bar = view->get_key_bar_info();

    size_t idx = 0;
    for (const auto key: refreshable_keys)
        for (const auto mod: { 0, SHIFT_PRESSED, LEFT_ALT_PRESSED, LEFT_CTRL_PRESSED })
        {
            auto &kbl = key_bar_labels[idx++];

            kbl.Key.VirtualKeyCode = key;
            kbl.Key.ControlKeyState = mod;

            if (view_key_bar && view_key_bar->contains(kbl.Key))
                kbl.Text = kbl.LongText = view_key_bar->at(kbl.Key).c_str();
            else
                kbl.Text = kbl.LongText = L"";
        }

    // allowing view to customize OpenPanelInfo struct
    return view->update_panel_info(info);
}

intptr_t panel::update_panel_items(GetFindDataInfo *info)
{
    // TODO: convert to ref?
    auto items = view->get_items();

    auto *panel_item = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items->size());
    if (panel_item == nullptr)
    {
        utils::log::global->error("Could not allocate memory for panel items");
        return FALSE;
    }

    for (size_t idx = 0; idx < items->size(); idx++)
    {
        const auto &item = (*items)[idx];
        const auto &columns = item.custom_column_data;

        // TODO: what if no memory allocated?
        auto **column_data = (const wchar_t**)malloc(sizeof(wchar_t*) * columns.size());
        for (size_t i = 0; i < columns.size(); i++)
            column_data[i] = _wcsdup(columns[i].c_str());
        
        memset(&panel_item[idx], 0, sizeof(PluginPanelItem));
        panel_item[idx].FileAttributes = item.file_attrs;
        panel_item[idx].Flags = PPIF_PROCESSDESCR;
        panel_item[idx].FileName = _wcsdup(utils::strip_invalid_filename_chars(item.name).c_str());
        panel_item[idx].Description = _wcsdup(item.description.c_str());
        panel_item[idx].CustomColumnData = column_data;
        panel_item[idx].CustomColumnNumber = item.custom_column_data.size();
        
        if (item.user_data != nullptr)
            panel_item[idx].UserData.Data = item.user_data;
    }

    view->on_items_updated();

    info->PanelItem = panel_item;
    info->ItemsNumber = items->size();

    return TRUE;
}

void panel::free_panel_items(const FreeFindDataInfo *info)
{
    for (size_t i = 0; i < info->ItemsNumber; i++)
    {
        auto &item = info->PanelItem[i];

        free(const_cast<wchar_t*>(item.FileName));
        free(const_cast<wchar_t*>(item.Description));

        for (size_t j = 0; j < info->PanelItem[i].CustomColumnNumber; j++)
            free(const_cast<wchar_t*>(item.CustomColumnData[j]));
        free(const_cast<wchar_t**>(item.CustomColumnData));
    }
    free(info->PanelItem);
}

intptr_t panel::select_directory(const SetDirectoryInfo *info)
{
    return view->select_item(info);
}

intptr_t panel::process_input(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        auto key = keys::make_combined(key_event);
        switch (key)
        {
            case VK_F3:
            {
                bool should_refresh = false;
                for (const auto &ppi: far3::panels::get_items(PANEL_ACTIVE, true))
                    if (view->request_extra_info(ppi.get()))
                        should_refresh = true;

                if (should_refresh) // refreshing only in case something has changed
                    refresh_panels();
        
                // blocking F3 panel processing in general, as we have a custom one
                return TRUE;
            }
            case VK_F12 + keys::mods::ctrl:
            {
                auto sort_modex_idx = show_sort_dialog(*view);
                if (sort_modex_idx > -1)
                    view->select_sort_mode(sort_modex_idx);
                return TRUE;
            }
        }

        if (view->process_input(info))
            return TRUE;

        // the sorting hotkeys are blocked, due to custom plugin implementation
        for (int key_code = VK_F3; key_code <= VK_F12; key_code++)
            if (key == key_code + keys::mods::ctrl)
                return TRUE;
    }
    return FALSE;
}

intptr_t panel::compare_items(const CompareInfo *info)
{
    return view->compare_items(info);
}

void panel::show_panel_view(std::shared_ptr<ui::view> v)
{
    view = v;
}

void panel::refresh_panels(const string &item_id)
{
    far3::panels::update(PANEL_ACTIVE);

    if (!item_id.empty())
    {
        if (auto item_idx = view->get_item_idx(item_id))
            far3::panels::redraw(PANEL_ACTIVE, item_idx, -1);
    }

    far3::panels::redraw(PANEL_ACTIVE);
}

} // namespace ui
} // namespace spotifar