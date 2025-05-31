#include "artists.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using namespace spotify;
using utils::far3::get_text;

//-----------------------------------------------------------------------------------------------------------
artists_base_view::artists_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, return_callback_t callback):
    view_abstract(panel, title, callback), api_proxy(api)
    {}

void artists_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Albums", L"Followers", L"Pop %" };
    modes[3].ColumnTypes = L"NON,C3,C0,C1";
    modes[3].ColumnWidths = L"0,6,9,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    modes[4].ColumnTypes = L"NON,C3";
    modes[4].ColumnWidths = L"0,6";
    modes[4].ColumnTitles = titles_3;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Name", L"Albums", L"Followers", L"Pop %", L"Genre" };
    modes[5].ColumnTypes = L"NON,C3,C0,C1,C2";
    modes[5].ColumnWidths = L"0,6,9,5,25";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { L"Name", L"Genres" };
    modes[6].ColumnTitles = titles_6;
    modes[6].StatusColumnTypes = NULL;
    modes[6].StatusColumnWidths = NULL;

    static const wchar_t* titles_7[] = { L"Name", L"Albums", L"Genres" };
    modes[7].ColumnTitles = titles_7;
    modes[7].ColumnTypes = L"NON,C3,Z";
    modes[7].ColumnWidths = L"30,6,0";
    modes[7].StatusColumnTypes = NULL;
    modes[7].StatusColumnWidths = NULL;
    modes[7].Flags = PMFLAGS_FULLSCREEN;

    modes[8] = modes[5]; // the same as 5th, but not fullscreen
    modes[8].Flags &= ~PMFLAGS_FULLSCREEN;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view_abstract::items_t& artists_base_view::get_items()
{
    static view_abstract::items_t items; items.clear();

    for (const auto &artist: get_artists())
    {
        std::vector<wstring> columns;

        // column C0 - followers count
        auto followers = artist.followers_total;
        if (followers < 1000000)
            columns.push_back(std::format(L"{:9}", followers));
        else if (followers < 1000000000)
            columns.push_back(std::format(L"{:7.2f} M", followers / 1000000.0));
        else if (followers < 1000000000000)
            columns.push_back(std::format(L"{:7.2f} B", followers / 1000000000.0));

        // column C1 - popularity
        columns.push_back(std::format(L"{:5}", artist.popularity));

        // column C2 - main genre
        columns.push_back(artist.get_main_genre());
        
        // column C3 - total albums
        wstring albums_count = L"";
        if (auto api = api_proxy.lock())
        {
            auto albums = api->get_artist_albums(artist.id);
        
            if (auto total_albums = albums->peek_total())
                albums_count = std::format(L"{: >6}", total_albums);
        }
        columns.push_back(albums_count);

        items.push_back({
            artist.id,
            artist.name,
            // here were use all artist's genres as a description field
            utils::to_wstring(utils::string_join(artist.genres, ", ")),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<artist_t*>(&artist)
        });
    }

    return items;
}

intptr_t artists_base_view::select_item(const data_item_t *data)
{
    if (const auto *artist = static_cast<const artist_t*>(data))
    {
        show_albums_view(*artist);
        return TRUE;
    }
    return FALSE;
}

bool artists_base_view::request_extra_info(const data_item_t *data)
{
    if (data != nullptr && !api_proxy.expired())
    {
        api_proxy.lock()->get_artist_albums(data->id)->get_total();
        return true;
    }
    return false;
}

const view_abstract::sort_modes_t& artists_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,            { VK_F3, LEFT_CTRL_PRESSED } },
        { L"Followers",     SM_SIZE,            { VK_F4, LEFT_CTRL_PRESSED } },
        { L"Popularity",    SM_COMPRESSEDSIZE,  { VK_F5, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

intptr_t artists_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const artist_t*>(data1),
        &item2 = static_cast<const artist_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_COMPRESSEDSIZE:
            return item1->popularity - item2->popularity;

        case SM_SIZE:
            return item1->followers_total - item2->followers_total;
    }
    return -2;
}


//-----------------------------------------------------------------------------------------------------------
followed_artists_view::followed_artists_view(HANDLE panel, api_weak_ptr_t api_proxy):
    artists_base_view(panel, api_proxy, get_text(MPanelArtistsItemLabel), std::bind(events::show_root, api_proxy))
{
    if (auto api = api_proxy.lock())
        collection = api->get_followed_artists();
}

config::settings::view_t followed_artists_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F3
    return { 0, false, 3 };
}

std::generator<const artist_t&> followed_artists_view::get_artists()
{
    if (collection && collection->fetch())
        for (const auto &a: *collection)
            co_yield a;
}

void followed_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist_albums(api_proxy, artist,
        std::bind(events::show_collection, api_proxy));
}

void followed_artists_view::show_filters_dialog()
{
    // TODO: unfinished, not for the aplha release
    /*PluginDialogBuilder builder(config::ps_info, MainGuid, FarMessageGuid, L"Test Dialog", NULL);

    std::unordered_set<string> genres;

    for (const auto &artist: *collection)
        genres.insert(artist.genres.begin(), artist.genres.end());

    std::vector<const wchar_t*> items;
    for (const auto &genre: genres)
        items.push_back(_wcsdup(utils::to_wstring(genre).c_str()));

    std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return wcscmp(a, b) < 0; });
        
    int selected_item;
    builder.AddListBox(&selected_item, 40, 10, &items[0], items.size(), DIF_LISTNOBOX);

    builder.AddOKCancel(MOk, MCancel);

    auto result = builder.ShowDialogEx();
    
    for (const auto &item: items)
        free(const_cast<wchar_t*>(item));*/
}


//-----------------------------------------------------------------------------------------------------------
recent_artists_view::recent_artists_view(HANDLE panel, api_weak_ptr_t api):
    artists_base_view(panel, api, get_text(MPanelArtistsItemLabel), std::bind(events::show_root, api))
{
    utils::events::start_listening<play_history_observer>(this);

    rebuild_items();
}

recent_artists_view::~recent_artists_view()
{
    utils::events::stop_listening<play_history_observer>(this);

    items.clear();
}

config::settings::view_t recent_artists_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const view_abstract::sort_modes_t& recent_artists_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = artists_base_view::get_sort_modes();
        modes.push_back({ L"Played at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

void recent_artists_view::rebuild_items()
{
    items.clear();

    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();
    std::unordered_map<string, history_item_t> recent_artists;
    
    // we pick the earliest possible item among all the duplicates, for that
    // the reverse order is used
    auto play_history = api->get_play_history();
    for (auto it = play_history.rbegin(); it != play_history.rend(); ++it)
        if (it->track.artists.size() > 0)
            recent_artists[it->track.artists[0].id] = *it;

    if (recent_artists.size() > 0)
    {
        const auto &keys = std::views::keys(recent_artists);
        const auto &ids = item_ids_t(keys.begin(), keys.end());

        for (const auto &artist: api->get_artists(ids))
            items.push_back(history_artist_t{ {artist}, recent_artists[artist.id].played_at });
    }
}

intptr_t recent_artists_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_artist_t*>(data1),
            &item2 = static_cast<const history_artist_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return artists_base_view::compare_items(sort_mode, data1, data2);
}

std::generator<const artist_t&> recent_artists_view::get_artists()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist_albums(api_proxy, artist,
        std::bind(events::show_recents, api_proxy));
}

void recent_artists_view::on_items_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}

//-----------------------------------------------------------------------------------------------------------
recently_liked_tracks_artists_view::recently_liked_tracks_artists_view(HANDLE panel, api_weak_ptr_t api):
    artists_base_view(panel, api, get_text(MPanelArtistsItemLabel),
                      std::bind(events::show_root, api))
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_tracks();

    rebuild_items();
}

config::settings::view_t recently_liked_tracks_artists_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const view_abstract::sort_modes_t& recently_liked_tracks_artists_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = artists_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

void recently_liked_tracks_artists_view::rebuild_items()
{
    items.clear();
    
    if (collection && collection->fetch(false, true, 3))
    {
        std::set<item_id_t> used_ids{};

        for (const auto &track: *collection)
            if (track.artists.size() > 0)
            {
                const auto &artist = track.artists[0];
                if (!used_ids.contains(artist.id))
                {
                    used_ids.insert(artist.id);
                    items.push_back({ {artist}, track.added_at });
                }
            }
    }
}

intptr_t recently_liked_tracks_artists_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const recent_artist_t*>(data1),
            &item2 = static_cast<const recent_artist_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return artists_base_view::compare_items(sort_mode, data1, data2);
}

std::generator<const artist_t&> recently_liked_tracks_artists_view::get_artists()
{
    for (const auto &i: items)
        co_yield i;
}

void recently_liked_tracks_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist_albums(api_proxy, artist,
        std::bind(events::show_recently_liked_tracks, api_proxy));
}


//-----------------------------------------------------------------------------------------------------------
recently_saved_album_artists_view::recently_saved_album_artists_view(HANDLE panel, api_weak_ptr_t api):
    artists_base_view(panel, api, get_text(MPanelArtistsItemLabel), std::bind(events::show_browse, api))
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_albums();

    rebuild_items();
}

config::settings::view_t recently_saved_album_artists_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const view_abstract::sort_modes_t& recently_saved_album_artists_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = artists_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

void recently_saved_album_artists_view::rebuild_items()
{
    items.clear();
    
    if (collection && collection->fetch(false, true, 3))
    {
        std::set<item_id_t> used_ids{};

        // we pick the earliest possible item among all the duplicates, for that
        // the reverse order is used
        for (auto it = collection->rbegin(); it != collection->rend(); ++it)
            if (it->artists.size() > 0)
            {
                const auto &artist = it->artists[0];
                if (!used_ids.contains(artist.id))
                {
                    used_ids.insert(artist.id);
                    items.push_back({ {artist}, it->added_at });
                }
            }
    }
}

intptr_t recently_saved_album_artists_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const recent_artist_t*>(data1),
            &item2 = static_cast<const recent_artist_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return artists_base_view::compare_items(sort_mode, data1, data2);
}

std::generator<const artist_t&> recently_saved_album_artists_view::get_artists()
{
    for (const auto &i: items)
        co_yield i;
}

void recently_saved_album_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist_albums(api_proxy, artist,
        std::bind(events::show_recently_saved_albums, api_proxy));
}


//-----------------------------------------------------------------------------------------------------------
user_top_artists_view::user_top_artists_view(HANDLE panel, api_weak_ptr_t api):
    artists_base_view(panel, api, get_text(MPanelArtistsItemLabel), std::bind(events::show_browse, api))
{
    if (auto api = api_proxy.lock())
        collection = api->get_user_top_artists();
}

config::settings::view_t user_top_artists_view::get_default_settings() const
{
    return { 1, false, 1 };
}

const view_abstract::sort_modes_t& user_top_artists_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Unsorted", SM_UNSORTED, { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

std::generator<const artist_t&> user_top_artists_view::get_artists()
{
    if (collection && collection->fetch(false, true, 4))
        for (const auto &a: *collection)
            co_yield a;
}

void user_top_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist_albums(api_proxy, artist,
        std::bind(events::show_user_top_items, api_proxy));
}


} // namespace ui
} // namespace spotifar