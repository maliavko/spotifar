#include "playlists.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
const view_abstract::items_t& playlists_base_view::get_items()
{
    static view_abstract::items_t items; items.clear();

    for (const auto &playlist: get_playlists())
    {
        std::vector<wstring> columns;

        // column C0 - total playlist's tracks count
        columns.push_back(std::format(L"{: >6}", playlist.tracks_total));

        // column C1 - owner
        columns.push_back(std::format(L"{: >15}", playlist.user_display_name));

        // column C2 - is public
        columns.push_back(playlist.is_public ? L" + " : L"");

        // column C3 - is collaborative
        columns.push_back(playlist.is_collaborative ? L" + " : L"");

        // column C4 - album length
        size_t total_length_ms = 0;
        if (auto api = api_proxy.lock())
        {
            // collecting the data only from cache if exists
            if (auto tracks = api->get_playlist_tracks(playlist.id); tracks->fetch(true, false))
                for (const auto &t: *tracks)
                    total_length_ms += t.duration_ms;
        }

        if (total_length_ms > 0)
            columns.push_back(std::format(L"{:>10%Hh %Mm}", std::chrono::milliseconds(total_length_ms)));
        else
            columns.push_back(L"");

        items.push_back({
            playlist.id,
            playlist.name,
            playlist.description,
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<simplified_playlist_t*>(&playlist)
        });
    }

    return items;
}

const view_abstract::sort_modes_t& playlists_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,    { VK_F3, LEFT_CTRL_PRESSED } },
        { L"Tracks",    SM_SIZE,    { VK_F4, LEFT_CTRL_PRESSED } },
        { L"Owner",     SM_OWNER,   { VK_F5, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

intptr_t playlists_base_view::select_item(const data_item_t* data)
{
    if (auto api = api_proxy.lock())
    {
        if (const auto &playlist = api->get_playlist(data->id))
        {
            events::show_playlist(api_proxy, playlist);
            return TRUE;
        }
    }
    return FALSE;
}

void playlists_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Tracks", L"Length", L"Owner", L"Pub", L"Col" };
    modes[3].ColumnTypes = L"NON,C0,C4,C1,C2,C3";
    modes[3].ColumnWidths = L"0,6,10,15,3,3";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Name", L"Description" };
    modes[4].ColumnTypes = L"NON,Z";
    modes[4].ColumnWidths = L"40,0";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Name", L"Tracks", L"Length", L"Owner", L"Pub", L"Col", L"Description" };
    modes[5].ColumnTypes = L"NON,C0,C4,C1,C2,C3,Z";
    modes[5].ColumnWidths = L"0,6,10,15,3,3,0";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    modes[6] = modes[3];
    
    modes[7] = modes[5];
    
    modes[8] = modes[3];
    
    modes[9] = modes[3];
    
    modes[0] = modes[3];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

intptr_t playlists_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const simplified_playlist_t*>(data1),
        &item2 = static_cast<const simplified_playlist_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME: // by playlist name
            return item1->name.compare(item2->name);

        case SM_SIZE: // by tracks count
            if (item1->tracks_total != item2->tracks_total)
                return item1->tracks_total < item2->tracks_total ? -1 : 1;
            return 0;

        case SM_OWNER: // by owner name
            return item1->user_display_name.compare(item2->user_display_name);
    }
    return -2;
}

intptr_t playlists_base_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_RETURN + utils::keys::mods::shift:
        {
            if (auto item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    // if (start_playback(user_data->id))
                    // {
                    //     events::show_player();
                    //     return TRUE;
                    // }
                    
                    if (auto api = api_proxy.lock())
                    {
                        const auto playlist = static_cast<const simplified_playlist_t*>(user_data);
                        api->start_playback(playlist->get_uri());
                        return TRUE;
                    }
                }
            }
            else
                log::global->error("There is an error occured while getting a current panel item");

            return TRUE;
        }
    }
    return FALSE;
}

bool playlists_base_view::request_extra_info(const data_item_t *data)
{
    if (auto api = api_proxy.lock(); api && data)
        return api->get_playlist_tracks(data->id)->fetch();

    return false;
}


//-----------------------------------------------------------------------------------------------------------
saved_playlists_view::saved_playlists_view(HANDLE panel, api_weak_ptr_t api_proxy):
    playlists_base_view(panel, api_proxy, get_text(MPanelPlaylistsItemLabel), std::bind(events::show_root, api_proxy)),
    api_proxy(api_proxy)
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_playlists();
}

config::settings::view_t saved_playlists_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F3
    return { 0, false, 3 };
}

std::generator<const simplified_playlist_t&> saved_playlists_view::get_playlists()
{
    if (collection && collection->fetch())
        for (const auto &p: *collection)
            co_yield p;
}


//----------------------------------------------------------------------------------------------------------
recent_playlists_view::recent_playlists_view(HANDLE panel, api_weak_ptr_t api):
    playlists_base_view(panel, api, get_text(MPanelPlaylistsItemLabel),
                        std::bind(events::show_root, api))
{
    rebuild_items();

    utils::events::start_listening<play_history_observer>(this);
}

recent_playlists_view::~recent_playlists_view()
{
    items.clear();

    utils::events::stop_listening<play_history_observer>(this);
}

config::settings::view_t recent_playlists_view::get_default_settings() const
{
    return { 0, false, 3 };
}

const view_abstract::sort_modes_t& recent_playlists_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = playlists_base_view::get_sort_modes();
        modes.push_back({ L"Played", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t recent_playlists_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_playlist_t*>(data1),
            &item2 = static_cast<const history_playlist_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return playlists_base_view::compare_items(sort_mode, data1, data2);
}

void recent_playlists_view::rebuild_items()
{
    items.clear();

    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();
    std::unordered_map<string, history_item_t> unique_recent_playlists;
    
    // collecting all the tracks' albums listened to
    for (const auto &item: api->get_play_history())
        if (item.context.is_playlist())
            unique_recent_playlists[item.context.get_item_id()] = item;

    for (const auto &[item_id, item]: unique_recent_playlists)
    {
        auto playlist = api->get_playlist(item_id);
        if (!playlist)
        {
            playlist.id = item_id;
            playlist.name = std::format(L"Forbidden_{}", utils::to_wstring(item_id));
        }

        items.push_back(history_playlist_t{ {playlist}, item.played_at });
    }
}

std::generator<const simplified_playlist_t&> recent_playlists_view::get_playlists()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_playlists_view::on_items_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}

} // namespace ui
} // namespace spotifar