#include "playlists.hpp"
#include "lng.hpp"
#include "ui/events.hpp"
#include "spotify/requesters.hpp"

namespace spotifar { namespace ui {

using PM = panel_mode_t;
using utils::far3::get_text;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
const items_t& playlists_base_view::get_items()
{
    static items_t items; items.clear();

    for (const auto &playlist: get_playlists())
    {
        std::vector<wstring> columns;

        // column C0 - total playlist's tracks count
        columns.push_back(utils::format(L"{: >6}", playlist.tracks_total));

        // column C1 - owner
        columns.push_back(utils::format(L"{: >15}", playlist.user_display_name));

        // column C2 - is public
        columns.push_back(playlist.is_public ? L" + " : L"");

        // column C3 - is collaborative
        columns.push_back(playlist.is_collaborative ? L" + " : L"");

        // column C4 - album length
        size_t total_length_ms = 0;
        if (auto api = api_proxy.lock())
        {
            // collecting the data only from cache if exists
            if (auto tracks = api->get_playlist_tracks(playlist.id); tracks->fetch(true, true))
                for (const auto &t: *tracks)
                    total_length_ms += t.duration_ms;
        }

        if (total_length_ms > 0)
            columns.push_back(utils::format(L"{:>10%Hh %Mm}", std::chrono::milliseconds(total_length_ms)));
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

const sort_modes_t& playlists_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarName),           SM_NAME,    { VK_F3, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarTracksCount),    SM_SIZE,    { VK_F4, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarOwner),          SM_OWNER,   { VK_F5, LEFT_CTRL_PRESSED } },
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

const panel_modes_t* playlists_base_view::get_panel_modes() const
{
    static const panel_mode_t::column_t
        Name        { L"NON",   get_text(MSortColName),         L"0" },
        NameFixed   { L"NON",   get_text(MSortColName),         L"30" },
        TracksCount { L"C0",    get_text(MSortColTracksCount),  L"6" },
        Owner       { L"C1",    get_text(MSortColOwner),        L"15" },
        IsPublic    { L"C2",    get_text(MSortColIsPublic),     L"3" },
        IsColab     { L"C3",    get_text(MSortColIsColab),      L"3" },
        Duration    { L"C4",    get_text(MSortColDuration),     L"10" },
        Descr       { L"Z",     get_text(MSortColDescr),        L"0" };
    
    static panel_modes_t modes{
        /* 0 */ PM::dummy(),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ &Name, &TracksCount, &Duration, &Owner, &IsPublic, &IsColab }),
        /* 4 */ PM({ &NameFixed, &Descr }),
        /* 5 */ PM({ &Name, &TracksCount, &Duration, &Owner, &IsPublic, &IsColab, &Descr }, true),
        /* 6 */ PM::dummy(3),
        /* 7 */ PM::dummy(5),
        /* 8 */ PM::dummy(3),
        /* 9 */ PM::dummy(3),
    };
    
    return &modes;
}

const key_bar_info_t* playlists_base_view::get_key_bar_info()
{
    static key_bar_info_t key_bar{};

    auto view_uid = get_uid();

    auto item = utils::far3::panels::get_current_item(get_panel_handle());

    // right after switching the directory on the panel, when we trap here `panels::get_current_item`
    // returns the `item` from  the previous directory. Checking crc32 helps to identify
    // when the panel is refreshed eventually and we can be sure the item is valid
    if (item->CRC32 != view_uid) return nullptr;
    
    if (auto api = api_proxy.lock())
    {
        // TODO: to implement a proper logic for saving playlists
        /*if (auto *user_data = unpack_user_data(item->UserData); user_data && user_data->is_valid())
        {
            if (api->get_library()->is_track_saved(user_data->id))
                key_bar[{ VK_F8, 0 }] = get_text(MUnlike);
            else
                key_bar[{ VK_F8, 0 }] = get_text(MLike);
        }*/
    }
    return &key_bar;
}

intptr_t playlists_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const simplified_playlist_t*>(data1),
        &item2 = static_cast<const simplified_playlist_t*>(data2);

    #if defined (__clang__)
    #   pragma clang diagnostic ignored "-Wswitch"
    #endif
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
    using namespace utils::keys;

    switch (combined_key)
    {
        case VK_F4:
        {
        // starting playback for the item or list of selected items
            if (auto item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (auto api = api_proxy.lock())
                    {
                        const auto playlist = static_cast<const simplified_playlist_t*>(user_data);
                        api->start_playback(playlist->get_uri());
                        return TRUE;
                    }
                }
            }
            else
            {
                log::global->error("There is an error occured while getting a current panel item");
            }
            return TRUE;
        }

        // redirect to Spotify WEB for the tracks under cursor
        case VK_RETURN + mods::shift:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (const auto *pl = static_cast<const simplified_playlist_t*>(user_data); !pl->urls.spotify.empty())
                        utils::open_web_browser(pl->urls.spotify);
                }
            }
            return TRUE;
        }
        
        // saving/removing playlists
        case VK_F8:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    // special logic for saving and removing hidden playlists
                    if (const auto *pl = static_cast<const simplified_playlist_t*>(user_data); pl && pl->is_hidden)
                    {
                        auto &playlists = config::get_hidden_playlists();
                        if (auto it = playlists.find(pl->id); it != playlists.end())
                        {
                            // removing
                            playlists.erase(it);
                        }
                        else
                        {
                            // adding
                            wchar_t playlist_name[64];

                            if (config::ps_info.InputBox(
                                &MainGuid, &FarMessageGuid, L"Saving playlist", L"Input the name for the playlist to save",
                                NULL, L"", playlist_name, 64, NULL, 0))
                            {
                                playlists[pl->id] = utils::utf8_encode(playlist_name);
                            }
                        }
                        // TODO: all this logic should be moved somewhere to library and hidden there,
                        // throwing the update events each time playlists saved into API or this
                        // special hidden cases, so all the interested listeners can get updated
                        // accordingly
                    }
                }
            }
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
    playlists_base_view(panel, api_proxy, get_text(MPanelPlaylists), get_text(MPanelCollection))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_saved_playlists();
        collection->fetch();
    }
}

config::settings::view_t saved_playlists_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F3
    return { 0, false, 3 };
}

std::generator<const simplified_playlist_t&> saved_playlists_view::get_playlists()
{
    static std::vector<simplified_playlist_t> hidden_playlists;

    hidden_playlists.clear();
    for (const auto &[pid, pname]: config::get_hidden_playlists())
        hidden_playlists.push_back(simplified_playlist_t::make_hidden(pid, utils::utf8_decode(pname)));
    
    if (collection)
        for (const auto &p: *collection) co_yield p;

    for (const auto &pl: hidden_playlists)
        co_yield pl;
}


//----------------------------------------------------------------------------------------------------------
recent_playlists_view::recent_playlists_view(HANDLE panel, api_weak_ptr_t api):
    playlists_base_view(panel, api, get_text(MPanelPlaylists), get_text(MPanelRecents))
{
    rebuild_items();
    utils::events::start_listening<play_history_observer>(this);
}

recent_playlists_view::~recent_playlists_view()
{
    utils::events::stop_listening<play_history_observer>(this);
    items.clear();
}

config::settings::view_t recent_playlists_view::get_default_settings() const
{
    return { 0, false, 3 };
}

const sort_modes_t& recent_playlists_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = playlists_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarPlayedAt), SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
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

        // it is very likely that the playlists is created by Spotify and
        // is hidden from API users
        if (!playlist)
        {
            playlist.id = item_id;
            playlist.is_hidden = true;

            auto hidden_playlists = config::get_hidden_playlists();
            if (auto it = hidden_playlists.find(item_id); it != hidden_playlists.end())
            {
                playlist.name = utils::utf8_decode(it->second);
            }
            else
            {
                playlist.name = utils::format(L"Hidden_{}", utils::to_wstring(item_id));
            }
        }

        items.push_back(history_playlist_t{ {playlist}, item.played_at });
    }
}

std::generator<const simplified_playlist_t&> recent_playlists_view::get_playlists()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_playlists_view::on_history_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}

} // namespace ui
} // namespace spotifar