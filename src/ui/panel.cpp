#include "ui/panel.hpp"
#include "ui/dialogs/menus.hpp"
#include "lng.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

// the `F` keys, which can be overriden by the nested views
static const std::array<int, 6> refreshable_keys = { VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8 };

/// @brief Shows a splash loading screen in the middle of the Far. If the `message` is not provided,
/// the default one is shown.
/// @note the message is closed automatically by the next panels redrawal
static void show_loading_splash(const wstring &message = L"")
{
    const wchar_t* msgs[] = { L"", L"" };

    if (message.empty())
        msgs[1] = L" Requesting data... ";
    else
        msgs[1] = message.c_str();

    config::ps_info.Message(&MainGuid, &SplashDialogGuid, 0, L"", msgs, std::size(msgs), 0);
}

panel::panel(api_proxy_ptr api):
    api_proxy(api)
{
    utils::events::start_listening<ui_events_observer>(this);
    utils::events::start_listening<requester_observer>(this);
}

panel::~panel()
{
    utils::events::stop_listening<ui_events_observer>(this);
    utils::events::stop_listening<requester_observer>(this);

    view.reset();
    api_proxy.reset();
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
    // TODO: refactor, no need to return nullptr, remove all the further checks afterwards
    auto items = view->get_items();

    auto *panel_item = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items->size());
    if (panel_item == nullptr)
    {
        log::global->error("Could not allocate memory for panel items");
        return FALSE;
    }

    for (size_t idx = 0; idx < items->size(); idx++)
    {
        const auto &item = (*items)[idx];
        const auto &columns = item.columns_data;

        // TODO: what if no memory allocated?
        auto **column_data = (const wchar_t**)malloc(sizeof(wchar_t*) * columns.size());
        for (size_t i = 0; i < columns.size(); i++)
            column_data[i] = _wcsdup(columns[i].c_str());

        auto attrs = item.file_attrs;
        if (item.is_selected)
            // the encrypted attribute is used to highlight currently playing items
            attrs |= FILE_ATTRIBUTE_ENCRYPTED;
        
        memset(&panel_item[idx], 0, sizeof(PluginPanelItem));
        panel_item[idx].FileAttributes = attrs;
        panel_item[idx].Flags = PPIF_PROCESSDESCR;
        panel_item[idx].FileName = _wcsdup(utils::strip_invalid_filename_chars(item.name).c_str());
        panel_item[idx].Description = _wcsdup(item.description.c_str());
        panel_item[idx].CustomColumnData = column_data;
        panel_item[idx].CustomColumnNumber = item.columns_data.size();
        
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
            // to request and show extra info on the panel for the item under cursor
            case VK_F3:
            {
                bool should_refresh = false;
                for (const auto &ppi: far3::panels::get_items(PANEL_ACTIVE, true))
                    if (view->request_extra_info(ppi.get()))
                        should_refresh = true;

                if (should_refresh) // refreshing only in case anything has changed
                    refresh_panels();
        
                // blocking F3 panel processing in general, as we have a custom one
                return TRUE;
            }
            // to show a sort menu for the currently opened view
            case VK_F12 + keys::mods::ctrl:
            {
                auto sort_modex_idx = show_sort_dialog(*view);
                if (sort_modex_idx > -1)
                    view->select_sort_mode(sort_modex_idx);
                
                return TRUE; // no need to show a standard sorting menu
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

void panel::show_panel_view(view_ptr v)
{
    view = v;
}

void panel::refresh_panels(const string &item_id)
{
    far3::panels::update(PANEL_ACTIVE);

    if (!item_id.empty())
    {
        if (auto item_idx = view->get_item_idx(item_id))
        {
            far3::panels::redraw(PANEL_ACTIVE, item_idx, -1);
            return;
        }
    }

    far3::panels::redraw(PANEL_ACTIVE);
}

/// @brief The requests, which do not require showing splash screen, as they are processed
/// in the background, hidden from user
static const std::set<string> no_splash_requests{
    "/v1/me/player/recently-played",
    "/v1/me/tracks/contains",
};

void panel::on_request_started(const string &url)
{
    using namespace utils::http;

    if (no_splash_requests.contains(trim_domain(trim_params(url))))
        return;
    
    // the handler is called only when complex http requests are being initiated, like
    // sync and async multipage collections fetching. In most of the cases it is done when
    // the new view is created and getting populated. So, we show a splash screen and it will
    // get closed automatically when the view initialization is done and the panel is redrawn
    show_loading_splash();
}

void panel::on_request_finished(const string &url)
{
}

void panel::on_request_progress_changed(const string &url, size_t progress, size_t total)
{
    using namespace utils::http;

    if (no_splash_requests.contains(trim_domain(trim_params(url))))
        return;
    
    show_loading_splash(std::format(L"Fetching progress: {}/{}", progress, total));
}

void panel::on_show_filters_menu()
{
    PluginDialogBuilder builder(config::ps_info, MainGuid, ConfigSpotifyDialogGuid, L"Test Dialog", NULL);

    builder.StartColumns();

    static int radio_idx;
    const int btns[] {
        MPanelAlbumsItemLabel, MPanelTracksItemLabel, MPanelAlbumItemLabel, MPanelPlaylistsItemLabel
    };
    builder.AddRadioButtons(&radio_idx, 4, btns);

    builder.ColumnBreak();

    static int radio_idx2;
    builder.AddRadioButtons(&radio_idx2, 4, btns);

    builder.EndColumns();

    builder.AddSeparator();

    static int selected_item;
    static const wchar_t* items[] = {
        L"11111111111 | 11111 | 1111",
        L"22222222222 | 22222 | 2222",
        L"33333333333 | 33333 | 3333",
    };
    builder.AddListBox(&selected_item, 40, 10, items, 3, DIF_LISTNOBOX);

    builder.AddOKCancel(MOk, MCancel);

    auto r = builder.ShowDialogEx();
    log::global->debug("dialog closed {} {} {}", r, radio_idx, radio_idx2);
}

} // namespace ui
} // namespace spotifar