#include "ui/panel.hpp"
#include "ui/dialogs/menus.hpp"
#include "ui/views/root.hpp"
#include "lng.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

// the `F` keys, which can be overriden by the nested views
static const std::array<int, 6> refreshable_keys = { VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8 };

/// @brief Shows a splash loading screen in the middle of the Far Manager's panels.
/// If the `message` is not provided, the default one is shown.
/// @note the message is closed automatically by the next panels redrawal
static void show_loading_splash(const wstring &message = L"")
{
    const wchar_t* msgs[] = { L"", L"" };

    if (message.empty())
        msgs[1] = L" Requesting data... "; // TODO: localize
    else
        msgs[1] = message.c_str();

    config::ps_info.Message(&MainGuid, &SplashDialogGuid, 0, L"", msgs, std::size(msgs), 0);
}

/// @brief A stub-view used for the first panel initialization, before any other
/// view to be shown. Purposely visible when the user is not yet authorized
class stub_view: public view_abstract
{
public:
    stub_view(HANDLE panel): view_abstract(panel, L"", nullptr) {}
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

panel::panel(api_weak_ptr_t api, plugin_ptr_t plugin):
    api_proxy(api), plugin_proxy(plugin)
{
    utils::events::start_listening<ui_events_observer>(this);
    utils::events::start_listening<api_requests_observer>(this);

    auto a = api.lock();
    if (a && a->is_authenticated())
        set_view(std::make_shared<root_view>(this, api_proxy));
    else
        set_view(std::make_shared<stub_view>(this));
}

panel::~panel()
{
    utils::events::stop_listening<ui_events_observer>(this);
    utils::events::stop_listening<api_requests_observer>(this);

    view.reset();
    api_proxy.reset();
    plugin_proxy.reset();
}

void panel::update_panel_info(OpenPanelInfo *info)
{
    static wchar_t dir_name[64], title[64];

    if (view == nullptr) return;
    
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
    if (const auto &info_lines = view->get_info_lines())
    {
        info->InfoLines = info_lines->data();
        info->InfoLinesNumber = info_lines->size();
    }

    // every update we clear out all the refreshable `F` keys and fill them up
    // by demand with the overriding info from the nested view
    static KeyBarLabel key_bar_labels[refreshable_keys.size() * 4];
    static KeyBarTitles key_bar = { std::size(key_bar_labels), key_bar_labels };
    info->KeyBar = &key_bar;

    view_abstract::key_bar_info_t panel_key_bar{};

    if (mview_builders.artists)
        panel_key_bar.insert({ { VK_F5, SHIFT_PRESSED }, far3::get_text(MPanelArtistsItemLabel) });

    if (mview_builders.albums)
        panel_key_bar.insert({ { VK_F6, SHIFT_PRESSED }, far3::get_text(MPanelAlbumsItemLabel) });

    if (mview_builders.tracks)
        panel_key_bar.insert({ { VK_F7, SHIFT_PRESSED }, far3::get_text(MPanelTracksItemLabel) });

    if (mview_builders.playlists)
        panel_key_bar.insert({ { VK_F8, SHIFT_PRESSED }, far3::get_text(MPanelPlaylistsItemLabel) });
    
    if (const auto &view_key_bar = view->get_key_bar_info())
        panel_key_bar.insert(view_key_bar->begin(), view_key_bar->end());

    size_t idx = 0;
    for (const auto &key: refreshable_keys)
        for (const auto &mod: { 0, SHIFT_PRESSED, LEFT_ALT_PRESSED, LEFT_CTRL_PRESSED })
        {
            auto &kbl = key_bar_labels[idx++];

            kbl.Key.VirtualKeyCode = key;
            kbl.Key.ControlKeyState = mod;

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

        for (size_t j = 0; j < info->PanelItem[i].CustomColumnNumber; j++)
            free(const_cast<wchar_t*>(item.CustomColumnData[j]));
        free(const_cast<wchar_t**>(item.CustomColumnData));
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
        auto key = keys::make_combined(key_event);
        switch (key)
        {
            // to request and show extra info on the panel for the item under cursor
            case VK_F3:
            {
                bool should_refresh = false;
                for (const auto &ppi: far3::panels::get_items(this, true))
                    if (view->request_extra_info(ppi.get()))
                        should_refresh = true;

                if (should_refresh) // refreshing only in case anything has changed
                    refresh();

                // blocking F3 panel processing in general, as we have a custom one
                return TRUE;
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
                auto idx = key - VK_F5 - keys::mods::shift;
                if (idx != mview_current_idx)
                {
                    if (auto builder = mview_builders.get_builder(idx))
                    {
                        mview_current_idx = idx;
                        set_view(builder(this));
                    }
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

void panel::set_view(view_ptr_t v)
{
    view = v; // setting up the new view

    // forcing the panel to redraw only in cases when it will not do it itself:
    //   - it is passive
    //   - it was not selected via SetDirectoryW function, which will redraw panels itself
    if (!is_active() || !skip_view_refresh)
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

void panel::refresh(const string &item_id) const
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
}

void panel::show_view(HANDLE panel, view_builder_t builder)
{
    if (is_this_panel(panel))
    {
        mview_builders.clear(); // removing any previously set filters
        set_view(builder(this));
    }
}

void panel::show_multiview(HANDLE panel, multiview_builder_t builders)
{
    if (is_this_panel(panel))
    {
        mview_builders = builders;
        mview_current_idx = builders.default_view_idx;

        if (auto builder = builders.get_builder(mview_current_idx))
            set_view(builder(this));
    }
}

void panel::close_panel(HANDLE panel)
{
    if (panel == NULL || is_this_panel(panel))
        far3::panels::quit(this);
}

void panel::refresh_panels(HANDLE panel, const string &item_id)
{
    if (panel == NULL || is_this_panel(panel))
        refresh(item_id);
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

    // the handler is called only when complex http requests are being initiated, like
    // sync and async multipage collections fetching. In most of the cases it is done when
    // the new view is created and getting populated. So, we show a splash screen and it will
    // get closed automatically when the view initialization is done and the panel is redrawn
    if (!no_splash_requests.contains(trim_domain(trim_params(url))))
        show_loading_splash();
}

void panel::on_request_finished(const string &url)
{
    // when any http request is being performed, panel shows a progress splash screen,
    // which gets hidden once all the received panel items are set, but FAR redraws only
    // the active panel, so half of the splash screen stays on the screen. Forcing the
    // passive panel to redraw as well
    far3::panels::redraw(PANEL_PASSIVE);
}

void panel::on_request_progress_changed(const string &url, size_t progress, size_t total)
{
    using namespace utils::http;

    if (!no_splash_requests.contains(trim_domain(trim_params(url))))
        show_loading_splash(std::format(L"Fetching progress: {}/{}", progress, total));
}

void panel::on_playback_command_failed(const string &message)
{
    utils::far3::show_far_error_dlg(MErrorPlaybackCmdFailed, utils::to_wstring(message));
}

void panel::on_collection_fetching_failed(const string &message)
{
    utils::far3::show_far_error_dlg(MErrorCollectionFetchFailed, utils::to_wstring(message));
}

} // namespace ui
} // namespace spotifar