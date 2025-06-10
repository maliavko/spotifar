#include "ui/panel.hpp"
#include "ui/dialogs/menus.hpp"
#include "ui/views/root.hpp"
#include "lng.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

// Macro-helper to define all possible key bar key binding combinations
#define ALL_COMBINATIONS(far_key) { far_key, 0 }, { far_key, SHIFT_PRESSED }, { far_key, LEFT_ALT_PRESSED }, { far_key, LEFT_CTRL_PRESSED }

// All the keys, used by plugin's views. If a current view does not define
// any bindings, all the keys below will be cleared from the panel
static const FarKey overriden_bindings[] = {
    ALL_COMBINATIONS(VK_F3),
    ALL_COMBINATIONS(VK_F4),
    ALL_COMBINATIONS(VK_F5),
    ALL_COMBINATIONS(VK_F6),
    ALL_COMBINATIONS(VK_F7),
    ALL_COMBINATIONS(VK_F8),
    { VK_F9, LEFT_CTRL_PRESSED },
    { VK_F10, LEFT_CTRL_PRESSED },
    { VK_F11, LEFT_CTRL_PRESSED },
};

/// @brief A stub-view used for the first panel initialization, before any other
/// view to be shown. Purposely visible when the user is not yet authorized
class stub_view: public view
{
public:
    stub_view(HANDLE panel): view(panel, L"") {}
protected:
    auto get_sort_modes() const -> const sort_modes_t& override
    {
        static const sort_modes_t sort_modes{};
        return sort_modes;
    };

    auto get_default_settings() const -> config::settings::view_t override
    {
        static const config::settings::view_t settings{};
        return settings;
    };
    
    auto get_items() -> const items_t& override
    {
        static const items_t items{};
        return items;
    }
};

panel::panel(plugin_ptr_t plugin_ptr): plugin_proxy(plugin_ptr)
{
    utils::events::start_listening<ui_events_observer>(this);

    auto api_ptr = plugin_ptr->get_api().lock();
    if (api_ptr && api_ptr->is_authenticated())
        set_view(std::make_shared<root_view>(this, api_ptr));
    else
        set_view(std::make_shared<stub_view>(this));
}

panel::~panel()
{
    utils::events::stop_listening<ui_events_observer>(this);

    view.reset();
    plugin_proxy.reset();
}

void panel::update_panel_info(OpenPanelInfo *info)
{
    static wchar_t dir_name[64], title[64];

    if (view == nullptr) return;
    
    const auto &view_title = view->get_title();
    if (!view_title.empty())
    {
        // showing "Spotifar: Menu Item Name" title in case a cur dir is not empty
        config::fsf.snprintf(title, std::size(title), L" %s: %s ",
            far3::get_text(MPluginUserName), view_title.c_str());
    }
    else
    {
        // ...or just "Spotifar" plugin name
        config::fsf.snprintf(title, std::size(title), far3::get_text(MPluginUserName));
    }
    
    config::fsf.snprintf(dir_name, std::size(dir_name), view->get_dir_name().c_str());

    info->StructSize = sizeof(*info);
    info->CurDir = dir_name;
    info->PanelTitle = title;
    info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING | OPIF_USECRC32;
    info->StartPanelMode = '3';
    info->StartSortMode = SM_NAME;

    // filling in the info lines on the Ctrl+L panel
    if (const auto &info_lines = view->get_info_lines())
    {
        info->InfoLines = info_lines->data();
        info->InfoLinesNumber = info_lines->size();
    }

    // every update we clear out all the overriden `F` key bindings and fill them up
    // using the current presented view and its binding configuration
    static KeyBarLabel key_bar_labels[std::size(overriden_bindings)];
    static KeyBarTitles key_bar = { std::size(key_bar_labels), key_bar_labels };
    info->KeyBar = &key_bar;

    // first, we collect all the key bindings into intermediate container
    view::key_bar_info_t panel_key_bar{};

    // adding multiviews bar keys
    if (mview_builders.artists)
        panel_key_bar[{ VK_F5, SHIFT_PRESSED }] = far3::get_text(MPanelArtistsItemLabel);

    if (mview_builders.albums)
        panel_key_bar[{ VK_F6, SHIFT_PRESSED }] = far3::get_text(MPanelAlbumsItemLabel);

    if (mview_builders.tracks)
        panel_key_bar[{ VK_F7, SHIFT_PRESSED }] = far3::get_text(MPanelTracksItemLabel);

    if (mview_builders.playlists)
        panel_key_bar[{ VK_F8, SHIFT_PRESSED }] = far3::get_text(MPanelPlaylistsItemLabel);
    
    // adding bar keys from the nested view
    if (const auto &view_key_bar = view->get_key_bar_info())
        panel_key_bar.insert(view_key_bar->begin(), view_key_bar->end());

    // adding sort bindings to the key bar
    for (const auto &sort_mode: view->get_sort_modes())
        panel_key_bar[sort_mode.far_key] = sort_mode.name.c_str();

    for (size_t idx = 0; idx < std::size(overriden_bindings); idx++)
    {
        auto &kbl = key_bar_labels[idx];

        kbl.Key = overriden_bindings[idx];

        // if the key is overriden right now, we show it on the bar;
        // otherwise we wipte it out from the panel
        if (panel_key_bar.contains(kbl.Key))
            kbl.Text = kbl.LongText = panel_key_bar.at(kbl.Key);
        else
            kbl.Text = kbl.LongText = L"";
    }

    // allowing view to customize OpenPanelInfo struct
    view->update_panel_info(info);
}

intptr_t panel::update_panel_items(GetFindDataInfo *info)
{
    skip_view_refresh = false;

    if (view == nullptr) return TRUE;

    auto view_crc32 = view->get_crc32();
    const auto &items = view->get_items();

    auto *panel_item = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items.size());
    if (panel_item == nullptr)
    {
        log::global->error("Could not allocate memory for panel items");
        return TRUE;
    }

    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        const auto &columns = item.columns_data;

        auto **column_data = (const wchar_t**)malloc(sizeof(wchar_t*) * columns.size());
        if (column_data == nullptr)
        {
            log::global->error("Could not allocate memory for panel columns");
            return TRUE;
        }

        for (size_t i = 0; i < columns.size(); i++)
            column_data[i] = columns[i].c_str();

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
        panel_item[idx].CRC32 = view_crc32; // emplacing the view's unique crc32 to be able to
                                            // distibguish which view the item belongs to later if needed
        
        if (item.user_data != nullptr)
            panel_item[idx].UserData.Data = item.user_data;
    }

    view->on_items_updated();

    info->PanelItem = panel_item;
    info->ItemsNumber = items.size();

    return TRUE;
}

void panel::free_panel_items(const FreeFindDataInfo *info)
{
    for (size_t i = 0; i < info->ItemsNumber; i++)
    {
        auto &item = info->PanelItem[i];

        free(const_cast<wchar_t*>(item.FileName));
        free(const_cast<wchar_t*>(item.Description));
    }
    free(info->PanelItem);
}

intptr_t panel::select_directory(const SetDirectoryInfo *info)
{
    skip_view_refresh = true;
    return view->select_item(info);
}

intptr_t panel::process_input(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (auto key = keys::make_combined(key_event))
        {
            // to request and show extra info on the panel for the item under cursor
            case VK_F3:
            {
                bool should_refresh = false;
                for (const auto &ppi: far3::panels::get_items(this, true))
                    if (view->request_extra_info(ppi.get()))
                        should_refresh = true;

                if (should_refresh) // refreshing only in case anything has changed
                {
                    refresh();
                    return TRUE;
                }
                break;
            }
            // to show a sort menu for the currently opened view
            case VK_F12 + keys::mods::ctrl:
            {
                const auto &modes = view->get_sort_modes();
                if (modes.size() == 0)
                    return TRUE;

                auto sort_modex_idx = (int)show_sort_dialog(*view);
                if (sort_modex_idx > -1)
                    view->select_sort_mode(sort_modex_idx);
                
                return TRUE; // no need to show a standard sorting menu
            }
            case VK_F5 + keys::mods::shift:
            case VK_F6 + keys::mods::shift:
            case VK_F7 + keys::mods::shift:
            case VK_F8 + keys::mods::shift:
            {
                // switch multiview
                auto idx = key - VK_F5 - keys::mods::shift;
                if (idx != mview_builders.settings->idx)
                {
                    if (auto builder = mview_builders.switch_builder(idx))
                        set_view(builder(this), view->get_return_callback());
                }
                return TRUE;
            }
        }

        // propagating the event processing routine to the nested view
        if (view->process_input(info))
            return TRUE;
    }
    return FALSE;
}

intptr_t panel::compare_items(const CompareInfo *info)
{
    return view->compare_items(info);
}

void panel::set_view(view_ptr_t v, view::return_callback_t callback)
{
    view = v; // setting up the new view

    if (callback)
        view->set_return_callback(callback);

    // if a user switches the directories on the panels, Far subsequently rebuilds panel items
    // and refreshes it, so we do not need to do it manually. In all other cases, if the view
    // is switched - we're manually refreshing the panel
    if (!skip_view_refresh)
        refresh();
}

bool panel::is_active() const
{
    return far3::panels::is_active((HANDLE)this);
}

bool panel::is_this_panel(HANDLE panel) const
{
    bool active = is_active();

    if ((panel == PANEL_ACTIVE && active) || (panel == PANEL_PASSIVE && !active))
        return true;

    return panel == this;
}

void panel::refresh(const string &item_id)
{
    // perform a redraw only in case there is an item to select
    if (!item_id.empty())
    {
        if (auto item_idx = view->get_item_idx(item_id))
        {
            far3::panels::redraw((HANDLE)this, item_idx, -1);
            return;
        }
    }
    else
    {
        // ...or perform a whole panels rebuild and redraw
        far3::panels::update((HANDLE)this);
        far3::panels::redraw((HANDLE)this);
    }

    const auto now = utils::clock_t::now();
    if (now - last_refresh_time < 200ms)
    {
        log::global->debug("!!!!!!!!!!!!!!!!!!!! last refresh was less than 1 second ago");
    }

    last_refresh_time = now;
}

void panel::show_view(HANDLE panel, view_builder_t builder, view::return_callback_t callback)
{
    if (is_this_panel(panel))
    {
        mview_builders.clear(); // removing any previously set filters
        set_view(builder(this), callback);
    }
}

void panel::show_multiview(HANDLE panel, multiview_builder_t builders, view::return_callback_t callback)
{
    if (is_this_panel(panel))
    {
        mview_builders = builders;

        if (auto builder = builders.get_builder())
            set_view(builder(this), callback);
    }
}

void panel::close_panel(HANDLE panel)
{
    if (panel == NULL || is_this_panel(panel))
        far3::panels::quit(this);
}

void panel::show_filters_menu()
{
    if (is_active() && view != nullptr)
        view->show_filters_dialog();
}

void panel::refresh_panels(HANDLE panel, const string &item_id)
{
    if (panel == NULL || is_this_panel(panel))
        refresh(item_id);
}

} // namespace ui
} // namespace spotifar