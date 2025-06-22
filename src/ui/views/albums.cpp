#include "albums.hpp"
#include "lng.hpp"
#include "ui/events.hpp"
#include "spotify/requesters.hpp"

namespace spotifar { namespace ui {

using mv_build_t = multiview_builder_t;
using PM = panel_mode_t;
using utils::far3::get_text;
namespace panels = utils::far3::panels;


//-----------------------------------------------------------------------------------------------------------
albums_base_view::albums_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name):
    view(panel, title, dir_name), api_proxy(api)
{
    utils::events::start_listening<collection_observer>(this);
}

albums_base_view::~albums_base_view()
{
    utils::events::stop_listening<collection_observer>(this);
    items.clear();
}

const items_t& albums_base_view::get_items()
{
    items.clear();

    for (const auto &album: get_albums())
    {
        std::vector<wstring> columns;
        
        size_t total_length_ms = 0;
        bool is_saved = false;

        if (auto api = api_proxy.lock())
        {
            auto tracks = api->get_album_tracks(album.id);
            // collecting the data only from cache if exists
            if (tracks->fetch(true, false))
                for (const auto &t: *tracks)
                    total_length_ms += t.duration_ms;

            auto *library = api->get_library();
            is_saved = library->is_album_saved(album.id);
        }
        
        // column C0 - release year
        columns.push_back(std::format(L"{: ^6}", utils::to_wstring(album.get_release_year())));
        
        // column C1 - album type
        columns.push_back(std::format(L"{: ^6}", album.get_type_abbrev()));
        
        // column C2 - total tracks
        columns.push_back(std::format(L"{:3}", album.total_tracks));

        // column C3 - full release date
        columns.push_back(std::format(L"{: ^10}", utils::to_wstring(album.release_date)));

        // column C4 - album length
        if (total_length_ms > 0)
            columns.push_back(std::format(L"{:%T >8}", std::chrono::milliseconds(total_length_ms)));
        else
            columns.push_back(L"");
        
        // column C5 - album's popularity
        columns.push_back(std::format(L"{:5}", album.popularity));

        // column C6 - main artist name
        columns.push_back(album.get_artist().name);

        // column C7 - is saved in collection status
        columns.push_back(is_saved ? L" + " : L"");

        // inherited views custom columns
        const auto &extra = get_extra_columns(album);
        columns.insert(columns.end(), extra.begin(), extra.end());

        items.push_back({
            album.id,
            album.name,
            album.get_main_copyright().text,
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<album_t*>(&album)
        });
    }
    return items;
}

const panel_modes_t* albums_base_view::get_panel_modes() const
{
    static const panel_mode_t::column_t
        ReleaseYear     { L"C0",    get_text(MSortColYear),         L"6" },
        Type            { L"C1",    get_text(MSortColType),         L"6" },
        TracksCount     { L"C2",    get_text(MSortColTracksCount),  L"4" },
        ReleaseDate     { L"C3",    get_text(MSortColRelease),      L"12" },
        TotalDuration   { L"C4",    get_text(MSortColDuration),     L"8" },
        Popularity      { L"C5",    get_text(MSortColPopularity),   L"5" },
        Artist          { L"C6",    get_text(MSortColArtist),       L"30" },
        Saved           { L"C7",    get_text(MSortColSaved),        L"3" },
        SavedAt         { L"C8",    get_text(MSortColSavedAt),      L"12" },
        Name            { L"NON",   get_text(MSortColName),         L"0" },
        Copyrights      { L"Z",     get_text(MSortColCopyrights),   L"0" };
        
    static panel_modes_t modes{
        /* 0 */ PM::dummy(4),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ &ReleaseYear, &Name, &Saved, &TracksCount, &TotalDuration, &Type, &Popularity }),
        /* 4 */ PM({ &ReleaseYear, &Saved, &Name, &Artist }),
        /* 5 */ PM({ &ReleaseYear, &Name, &Artist, &Saved, &TracksCount, &TotalDuration, &Type, &Popularity, &Copyrights }, true),
        /* 6 */ PM({ &ReleaseYear, &Name, &Saved, &Copyrights }),
        /* 7 */ PM({ &ReleaseYear, &Name, &Artist, &Saved, &Copyrights }, true),
        /* 8 */ PM({ &ReleaseYear, &Name, &Artist, &Saved, &TracksCount, &TotalDuration, &Type, &Popularity }),
        /* 9 */ PM::dummy(8),
    };
    
    return &modes;
}

const sort_modes_t& albums_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarName),       SM_NAME,            { VK_F3, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarRelease),    SM_ATIME,           { VK_F4, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarPopularity), SM_COMPRESSEDSIZE,  { VK_F5, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarTracksCount),SM_SIZE,            { VK_F6, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarArtist),     SM_OWNER,           { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

intptr_t albums_base_view::select_item(const data_item_t* data)
{
    if (const auto *album = static_cast<const album_t*>(data))
    {
        show_tracks_view(*album);
        return TRUE;
    }
    return FALSE;
}

intptr_t albums_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const album_t*>(data1),
        &item2 = static_cast<const album_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME: // by name
            return item1->name.compare(item2->name);

        case SM_ATIME: // by release date
            return item1->release_date.compare(item2->release_date);

        case SM_COMPRESSEDSIZE: // by popularity
            return item1->popularity - item2->popularity;

        case SM_SIZE: // by total tracks
            return item1->total_tracks - item2->total_tracks;

        case SM_OWNER: // by album name
            return item1->get_artist().name.compare(item2->get_artist().name);
    }
    return -2;
}

bool albums_base_view::request_extra_info(const data_item_t* data)
{
    // if the album's track list is cached, next time the panel
    // is updated, it will show album's total-length field
    if (auto api = api_proxy.lock(); api && data)
        return api->get_album_tracks(data->id)->fetch();

    return false;
}

intptr_t albums_base_view::process_key_input(int combined_key)
{
    using namespace utils::keys;

    switch (combined_key)
    {
        // launch a playback
        case VK_F4:
        {
            const auto &ids = get_selected_items();

            if (ids.size() == 1)
            {
                if (auto api = api_proxy.lock())
                {
                    // if only one items is selected, we launch it in the current folder context
                    log::global->info("Launching playback with the given album id {}", ids[0]);
                    api->start_playback(album_t::make_uri(ids[0]));
                }
            }
            else if (ids.size() > 1)
            {
                // TODO: not tested, finish after https://github.com/librespot-org/librespot/pull/1509
                log::global->info("Launching several albums playback {}", utils::string_join(ids, ","));
                if (auto api = api_proxy.lock())
                {
                    std::vector<string> tracks_uris;

                    for (const auto &album_id: ids)
                    {
                        if (auto album_tracks = api->get_album_tracks(album_id); album_tracks->fetch())
                        {
                            std::transform(album_tracks->begin(), album_tracks->end(), std::back_inserter(tracks_uris),
                                [](const auto &s_track) { return s_track.get_uri(); });
                        }
                        else
                        {
                            log::global->warn("Could not get the album tracks list, skipping");
                        }
                    }

                    if (!tracks_uris.empty())
                        api->start_playback(tracks_uris);
                }
            }
            return TRUE;
        }

        // saved/remove from collection
        case VK_F8:
        {
            const auto &ids = get_selected_items();

            if (auto api = api_proxy.lock(); !ids.empty())
            {
                // what to do - like or unlike - with the whole list of items
                // we decide based on the first item state
                if (auto *library = api->get_library(); library->is_album_saved(ids[0], true))
                    library->remove_saved_albums(ids);
                else
                    library->save_albums(ids);
            }

            return TRUE;
        }

        // redirect to Spotify WEB
        case VK_RETURN + mods::shift:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (const album_t *album = static_cast<const album_t*>(user_data); !album->urls.spotify.empty())
                        utils::open_web_browser(album->urls.spotify);
                }
            }
            return TRUE;
        }

        // PgDown + Ctrl
        // go to the albums's origin artist
        case VK_NEXT + mods::ctrl:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData); *user_data)
                {
                    const album_t *album = static_cast<const album_t*>(user_data);
                    if (auto api = api_proxy.lock(); const auto &a = api->get_artist(album->get_artist().id))
                        events::show_artist(api_proxy, a);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

const key_bar_info_t* albums_base_view::get_key_bar_info()
{
    static key_bar_info_t key_bar{};

    auto view_crc32 = get_uid();
    auto item = utils::far3::panels::get_current_item(get_panel_handle());

    // right after switching the directory on the panel, when we trap here `panels::get_current_item`
    // returns the `item` from  the previous directory. Checking crc32 helps to identify
    // when the panel is refreshed eventually and we can be sure the item is valid
    if (item->CRC32 != view_crc32) return nullptr;

    if (auto *user_data = unpack_user_data(item->UserData); auto api = api_proxy.lock())
    {
        auto *library = api->get_library();
        if (library->is_album_saved(user_data->id))
            key_bar[{ VK_F8, 0 }] = get_text(MUnlike);
        else
            key_bar[{ VK_F8, 0 }] = get_text(MLike);
    }

    return &key_bar;
}

void albums_base_view::on_albums_statuses_changed(const item_ids_t &ids)
{
    on_albums_statuses_received(ids);
}

void albums_base_view::on_albums_statuses_received(const item_ids_t &ids)
{
    std::unordered_set<item_id_t> unique_ids(ids.begin(), ids.end());

    const auto &it = std::find_if(items.begin(), items.end(),
        [&unique_ids](item_t &item) { return unique_ids.contains(item.id); });

    // if any of view's tracks are changed, we need to refresh the panel
    if (it != items.end())
        events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
artist_albums_view::artist_albums_view(HANDLE panel, api_weak_ptr_t api, const artist_t &a):
    albums_base_view(panel, api, a.name, a.name), artist(a)
{
    rebuild_items();
}

config::settings::view_t artist_albums_view::get_default_settings() const
{
    return { 1, true, 3 };
}

std::generator<const album_t&> artist_albums_view::get_albums()
{
    for (const auto &a: albums) co_yield a;
}

void artist_albums_view::show_tracks_view(const album_t &album) const
{
    // let's open a tracks list view for the selected album, which should return user to the same
    // artist's albums view
    events::show_album_tracks(
        api_proxy, album,
        [api = api_proxy, artist = artist, this]
        {
            events::show_artist(api, artist, mv_build_t::albums_idx);
        });
}

void artist_albums_view::rebuild_items()
{
    albums.clear();

    if (auto api = api_proxy.lock())
    {
        if (const auto &simple_albums = api->get_artist_albums(artist.id); simple_albums->fetch())
        {
            item_ids_t ids;
            std::transform(simple_albums->begin(), simple_albums->end(), back_inserter(ids),
                [](const auto &a) { return a.id; });

            albums = api->get_albums(ids);
        }
    }
}


//-----------------------------------------------------------------------------------------------------------
saved_albums_view::saved_albums_view(HANDLE panel, api_weak_ptr_t api_proxy):
    albums_base_view(panel, api_proxy, get_text(MPanelAlbums), get_text(MPanelCollection))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_library()->get_saved_albums();
        collection->fetch();
    }

    static const panel_mode_t::column_t
        SavedAt { L"C8", get_text(MSortColSavedAt), L"12" };

    panel_modes = *albums_base_view::get_panel_modes();
    panel_modes[4].insert_column(&SavedAt, 0);
    panel_modes[5].insert_column(&SavedAt, 0);
    panel_modes[7].insert_column(&SavedAt, 0);

    panel_modes.rebuild();
}

config::settings::view_t saved_albums_view::get_default_settings() const
{
    // sort mode - by `Saved at`; descending; view mode - F6
    return { 5, true, 3 };
}

std::generator<const album_t&> saved_albums_view::get_albums()
{
    if (collection)
        for (const auto &a: *collection) co_yield a;
}

const sort_modes_t& saved_albums_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarSavedAt), SM_MTIME, { VK_F8, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t saved_albums_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const saved_album_t*>(data1),
            &item2 = static_cast<const saved_album_t*>(data2);

        return item1->added_at.compare(item2->added_at);
    }
    return albums_base_view::compare_items(sort_mode, data1, data2);
}

void saved_albums_view::show_tracks_view(const album_t &album) const
{
    events::show_album_tracks(api_proxy, album, [this] { events::show_collection(api_proxy); });
}

std::vector<wstring> saved_albums_view::get_extra_columns(const album_t& album) const
{
    const auto &saved_album = static_cast<const saved_album_t&>(album);

    // we take first 10 symbols - it is date, the rest is time
    const auto &saved_at_str = std::format("{:^12}", saved_album.added_at.substr(0, 10));

    return {
        utils::to_wstring(saved_at_str), // C8 - `added at` date
    };
}

// void saved_albums_view::on_albums_statuses_changed(const item_ids_t &ids)
// {
//     // the base handlers update the view only in case some items
//     // were being change, this view should repopulate itself anyway
//     // as it represents the list of saved tracks
//     if (collection->fetch())
//         events::refresh_panel(get_panel_handle());
// }


//-----------------------------------------------------------------------------------------------------------
new_releases_view::new_releases_view(HANDLE panel, api_weak_ptr_t api_proxy):
    albums_base_view(panel, api_proxy, get_text(MPanelNewReleases))
{
    rebuild_items();
    utils::events::start_listening<releases_observer>(this);
}

new_releases_view::~new_releases_view()
{
    utils::events::stop_listening<releases_observer>(this);
    recent_releases.clear();
}

config::settings::view_t new_releases_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F8
    return { 1, false, 8 };
}

std::generator<const album_t&> new_releases_view::get_albums()
{
    for (const auto &i: recent_releases) co_yield i;
}

void new_releases_view::show_tracks_view(const album_t &album) const
{
    const auto &simple_artist = album.get_artist();
    if (auto api = api_proxy.lock(); api && simple_artist)
    {
        const auto &artist = api->get_artist(simple_artist.id);
        events::show_album_tracks(
            api_proxy, album,
            [api = api_proxy, artist = artist, this]
            {
                // TODO: caugth some crashes here, perhaps `get_return_callback` is not valid already
                events::show_artist(api, artist, mv_build_t::albums_idx, get_return_callback());
            });
    }
}

void new_releases_view::on_releases_sync_finished(const recent_releases_t releases)
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}

void new_releases_view::rebuild_items()
{
    recent_releases.clear();

    if (auto api = api_proxy.lock())
    {
        const auto &releases = api->get_releases()->get_items();
        
        item_ids_t releases_ids;
        std::transform(releases.begin(), releases.end(), back_inserter(releases_ids),
            [](const auto &r) { return r.id; });

        recent_releases = api->get_albums(releases_ids);
    }
}

//-----------------------------------------------------------------------------------------------------------
recent_albums_view::recent_albums_view(HANDLE panel, api_weak_ptr_t api):
    albums_base_view(panel, api, get_text(MPanelAlbums), get_text(MPanelRecents))
{
    rebuild_items();
    utils::events::start_listening<play_history_observer>(this);
}

recent_albums_view::~recent_albums_view()
{
    utils::events::stop_listening<play_history_observer>(this);
    items.clear();
}

config::settings::view_t recent_albums_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const sort_modes_t& recent_albums_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarPlayedAt), SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

std::generator<const album_t&> recent_albums_view::get_albums()
{
    for (const auto &i: items) co_yield i;
}

void recent_albums_view::rebuild_items()
{
    items.clear();

    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();
    std::unordered_map<string, history_item_t> recent_albums;
    
    // we pick the earliest possible item among all the duplicates, for that
    // the reverse order is used
    auto play_history = api->get_play_history();
    for (auto it = play_history.rbegin(); it != play_history.rend(); ++it)
        recent_albums[it->album.id] = *it;

    if (recent_albums.size() > 0)
    {
        const auto &keys = std::views::keys(recent_albums);
        const auto &ids = item_ids_t(keys.begin(), keys.end());

        for (const auto &album: api->get_albums(ids))
            items.push_back(history_album_t{ {album}, recent_albums[album.id].played_at });
    }
}

intptr_t recent_albums_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_album_t*>(data1),
            &item2 = static_cast<const history_album_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return albums_base_view::compare_items(sort_mode, data1, data2);
}

void recent_albums_view::show_tracks_view(const album_t &album) const
{
    const auto &simple_artist = album.get_artist();
    if (auto api = api_proxy.lock(); api && simple_artist)
    {
        const auto &artist = api_proxy.lock()->get_artist(simple_artist.id);
        events::show_album_tracks(
            api_proxy, album,
            [api = api_proxy, artist = artist, this]
            {
                events::show_artist(api, artist, mv_build_t::albums_idx, get_return_callback());
            });
    }
}

void recent_albums_view::on_history_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
recently_saved_albums_view::recently_saved_albums_view(HANDLE panel, api_weak_ptr_t api_proxy):
    albums_base_view(panel, api_proxy, get_text(MPanelAlbums), get_text(MPanelRecentlySaved))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_library()->get_saved_albums();
        repopulate();
    }

    static const panel_mode_t::column_t
        SavedAt { L"C8", get_text(MSortColSavedAt), L"12" };

    panel_modes = *albums_base_view::get_panel_modes();
    panel_modes[4].insert_column(&SavedAt, 0);
    panel_modes[5].insert_column(&SavedAt, 0);
    panel_modes[7].insert_column(&SavedAt, 0);

    panel_modes.rebuild();
}

bool recently_saved_albums_view::repopulate()
{
    return collection->fetch(false, true, 3);
}

config::settings::view_t recently_saved_albums_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const sort_modes_t& recently_saved_albums_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarSavedAt), SM_MTIME, { VK_F8, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t recently_saved_albums_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const saved_album_t*>(data1),
            &item2 = static_cast<const saved_album_t*>(data2);

        return item1->added_at.compare(item2->added_at);
    }
    return albums_base_view::compare_items(sort_mode, data1, data2);
}

void recently_saved_albums_view::show_tracks_view(const album_t &album) const
{
    events::show_album_tracks(api_proxy, album,
        [api = api_proxy] { events::show_recently_saved(api); });
}

std::generator<const album_t&> recently_saved_albums_view::get_albums()
{
    if (collection)
        for (const auto &a: *collection) co_yield a;
}

std::vector<wstring> recently_saved_albums_view::get_extra_columns(const album_t& album) const
{
    const auto &saved_album = static_cast<const saved_album_t&>(album);

    // we take first 10 symbols - it is date, the rest is time
    const auto &saved_at_str = std::format("{:^12}", saved_album.added_at.substr(0, 10));

    return {
        utils::to_wstring(saved_at_str), // C8 - `added at` date
    };
}

// void recently_saved_albums_view::on_albums_statuses_changed(const item_ids_t &ids)
// {
//     // the base handlers update the view only in case some items
//     // were being change, this view should repopulate itself anyway
//     // as it represents the list of saved tracks
//     if (repopulate())
//         events::refresh_panel(get_panel_handle());
// }

} // namespace ui
} // namespace spotifar