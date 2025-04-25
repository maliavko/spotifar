#include "albums.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
albums_base_view::albums_base_view(api_proxy_ptr api, const string &view_uid,
                                   const wstring &title, return_callback_t callback):
    view_abstract(view_uid, title, callback), api_proxy(api)
    {}

const view_abstract::sort_modes_t& albums_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Release Year",  SM_ATIME,   VK_F4 + mods::ctrl },
        { L"Tracks",        SM_SIZE,    VK_F5 + mods::ctrl },
    };
    return modes;
}

intptr_t albums_base_view::select_item(const data_item_t* data)
{
    const auto *album = static_cast<const album_t*>(data);
    if (album != nullptr)
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
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_ATIME:
            return item1->release_date.compare(item2->release_date);

        case SM_SIZE:
            return item1->total_tracks - item2->total_tracks;
    }
    return -2;
}

void albums_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Yr", L"Name", L"Tx", L"Time", L"Type" };
    modes[3].ColumnTypes = L"C0,NON,C2,C4,C1";
    modes[3].ColumnWidths = L"6,0,4,8,6";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Yr", L"Name", L"Tx" };
    modes[4].ColumnTypes = L"C0,NON,C2";
    modes[4].ColumnWidths = L"6,0,4";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Yr", L"Name", L"Tx", L"Type", L"Release" };
    modes[5].ColumnTypes = L"C0,NON,C2,C1,C3";
    modes[5].ColumnWidths = L"6,0,4,6,10";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { L"Yr", L"Name", L"Artists" };
    modes[6].ColumnTypes = L"C0,NON,Z";
    modes[6].ColumnWidths = L"6,0,0";
    modes[6].ColumnTitles = titles_6;

    modes[7] = modes[6];
    modes[7].Flags = PMFLAGS_FULLSCREEN;

    modes[8] = modes[5]; // the same as 5th, but not fullscreen
    modes[8].Flags &= ~PMFLAGS_FULLSCREEN;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

bool albums_base_view::request_extra_info(const data_item_t* data)
{
    if (data != nullptr && !api_proxy.expired())
        return api_proxy.lock()->get_album_tracks(data->id)->fetch();

    return false;
}

intptr_t albums_base_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_RETURN + utils::keys::mods::shift:
        {
            auto item = utils::far3::panels::get_current_item(PANEL_ACTIVE);
            if (item != nullptr && !api_proxy.expired())
            {
                auto *user_data = unpack_user_data(item->UserData);
                log::global->info("Starting playback from the panel, {}", user_data->id);
                api_proxy.lock()->start_playback(album_t::make_uri(user_data->id));
            }
            else
                log::global->error("There is an error occured while getting a current panel item");

            return TRUE;
        }
    }
    return FALSE;
}

const view_abstract::items_t* albums_base_view::get_items()
{
    static items_t items; items.clear();

    for (const auto &a: get_albums())
    {
        std::vector<wstring> columns;
        
        // TODO: saved-or-not column?
        
        // column C0 - release year
        columns.push_back(std::format(L"{: ^6}", utils::to_wstring(a.get_release_year())));
        
        // column C1 - album type
        columns.push_back(std::format(L"{: ^6}", a.get_type_abbrev()));
        
        // column C2 - total tracks
        columns.push_back(std::format(L"{:3}", a.total_tracks));

        // column C3 - full release date
        columns.push_back(std::format(L"{: ^10}", utils::to_wstring(a.release_date)));

        // column C4 - album length
        size_t total_length_ms = 0;
        if (auto api = api_proxy.lock())
        {
            auto tracks = api->get_album_tracks(a.id);
            if (tracks->fetch(true))
            {
                for (const auto &t: *tracks)
                    total_length_ms += t.duration_ms;
            }
        }

        // TODO: column C5 - copyrights
        // album popularity
        // music label
        // external ids

        if (total_length_ms > 0)
            columns.push_back(std::format(L"{:%T >8}", std::chrono::milliseconds(total_length_ms)));
        else
            columns.push_back(L"");
        
        // list of artists is used as a description field
        std::vector<wstring> artists_names;
        std::transform(a.artists.cbegin(), a.artists.cend(), back_inserter(artists_names),
            [](const auto &a) { return a.name; });

        items.push_back({
            a.id,
            a.name,
            utils::string_join(artists_names, L", "),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<simplified_album_t*>(&a)
        });
    }
    return &items;
}

//-----------------------------------------------------------------------------------------------------------
artist_view::artist_view(api_proxy_ptr api, const artist_t &a, return_callback_t callback):
    albums_base_view(api, "artist_view", a.name, callback),
    artist(a)
{
    if (auto api = api_proxy.lock())
        collection = api->get_artist_albums(a.id);
}

config::settings::view_t artist_view::get_default_settings() const
{
    return { 1, false, 3 };
}

std::generator<const simplified_album_t&> artist_view::get_albums()
{
    // TODO: the pattern is copied bunch of times, reconsider the code, remove duplicates
    if (collection && collection->fetch())
        for (const auto &a: *collection)
            co_yield a;
}

void artist_view::show_tracks_view(const album_t &album) const
{
    // let's open a tracks list view for the selected album, which should return user to the same
    // artist's albums view
    events::show_album_tracks(api_proxy, album,
        std::bind(events::show_artist_albums, api_proxy, artist, get_return_callback()));
}

//-----------------------------------------------------------------------------------------------------------
saved_albums_view::saved_albums_view(api_proxy_ptr api_proxy):
    albums_base_view(api_proxy, "saved_albums_view", get_text(MPanelAlbumsItemLabel),
        std::bind(events::show_collections, api_proxy))
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_albums();
}

config::settings::view_t saved_albums_view::get_default_settings() const
{
    return { 1, false, 6 };
}

std::generator<const simplified_album_t&> saved_albums_view::get_albums()
{
    if (collection && collection->fetch())
        for (const auto &a: *collection)
            co_yield a;
}

const view_abstract::sort_modes_t& saved_albums_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ L"Added", SM_MTIME, VK_F6 + mods::ctrl });
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
    events::show_album_tracks(api_proxy, album,
        std::bind(events::show_saved_albums, api_proxy));
}

//-----------------------------------------------------------------------------------------------------------
new_releases_view::new_releases_view(api_proxy_ptr api_proxy):
    albums_base_view(api_proxy, "new_releases_view", get_text(MPanelNewReleasesItemLabel),
        std::bind(events::show_browse, api_proxy))
{
    if (auto api = api_proxy.lock())
        collection = api->get_new_releases();
}

config::settings::view_t new_releases_view::get_default_settings() const
{
    return { 1, false, 6 };
}

std::generator<const simplified_album_t&> new_releases_view::get_albums()
{
    if (collection && collection->fetch())
        for (const auto &a: *collection)
            co_yield a;
}

void new_releases_view::show_tracks_view(const album_t &album) const
{
    if (album.artists.size() > 0 && !api_proxy.expired())
    {
        const auto &artist = api_proxy.lock()->get_artist(album.artists[0].id);
        events::show_album_tracks(api_proxy, album,
            std::bind(events::show_artist_albums, api_proxy, artist, get_return_callback()));
    }
}

//-----------------------------------------------------------------------------------------------------------
recent_albums_view::recent_albums_view(api_proxy_ptr api):
    albums_base_view(api, "recent_albums_view", get_text(MPanelAlbumsItemLabel),
        std::bind(events::show_recents, api))
{
    utils::events::start_listening<play_history_observer>(this);

    rebuild_items();
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

const view_abstract::sort_modes_t& recent_albums_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ L"Played at", SM_MTIME, VK_F6 + mods::ctrl });
    }
    return modes;
}

std::generator<const simplified_album_t&> recent_albums_view::get_albums()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_albums_view::rebuild_items()
{
    items.clear();

    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();
    std::unordered_map<string, history_item_t> recent_albums;
    
    // collecting all the tracks' albums listened to
    for (const auto &item: api->get_play_history())
        recent_albums[item.track.album.id] = item;

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
    if (album.artists.size() > 0 && !api_proxy.expired())
    {
        const auto &artist = api_proxy.lock()->get_artist(album.artists[0].id);
        events::show_album_tracks(api_proxy, album,
            std::bind(events::show_artist_albums, api_proxy, artist, get_return_callback()));
    }
}

void recent_albums_view::on_items_changed()
{
    rebuild_items();
    
    panels::update(PANEL_ACTIVE);
    panels::redraw(PANEL_ACTIVE);
}

//-----------------------------------------------------------------------------------------------------------
featuring_albums_view::featuring_albums_view(api_proxy_ptr api):
    albums_base_view(api, "featuring_albums_view", get_text(MPanelFeaturingAlbumsItemLabel),
        std::bind(events::show_browse, api))
    {}

config::settings::view_t featuring_albums_view::get_default_settings() const
{
    return { 1, false, 6 };
}

std::generator<const simplified_album_t&> featuring_albums_view::get_albums()
{
    // if (collection->fetch())
    //     for (const auto &a: *collection)
    //         co_yield a;
    co_return;
}

void featuring_albums_view::show_tracks_view(const album_t &album) const
{
    if (album.artists.size() > 0 && !api_proxy.expired())
    {
        const auto &artist = api_proxy.lock()->get_artist(album.artists[0].id);
        events::show_album_tracks(api_proxy, album,
            std::bind(events::show_artist_albums, api_proxy, artist, get_return_callback()));
    }
}


} // namespace ui
} // namespace spotifar