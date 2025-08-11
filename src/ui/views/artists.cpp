#include "artists.hpp"
#include "lng.hpp"
#include "ui/events.hpp"
#include "spotify/requesters.hpp"

namespace spotifar { namespace ui {

using mv_build_t = multiview_builder_t;
using PM = panel_mode_t;
using utils::far3::get_text;
using namespace spotify;
namespace panels = utils::far3::panels;

static wstring format_followers(uintmax_t followers)
{
    return utils::to_wstring(utils::format_number(followers, 1000, " KMGTPE", 100.));
}

static std::vector<string> get_album_types_filters()
{
    std::vector<string> groups;
    
    auto filters = config::get_filters_settings();
    if (filters.albums_lps)
        groups.push_back("album");
    if (filters.albums_eps)
        groups.push_back("single");
    if (filters.albums_appears_on)
        groups.push_back("appears_on");
    if (filters.albums_compilations)
        groups.push_back("compilation");

    return groups;
}

//-----------------------------------------------------------------------------------------------------------
artists_base_view::artists_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name):
    view(panel, title, dir_name), api_proxy(api)
{
    utils::events::start_listening<collection_observer>(this);
}

artists_base_view::~artists_base_view()
{
    utils::events::stop_listening<collection_observer>(this);
    api_proxy.reset();
}

const items_t& artists_base_view::get_items()
{
    items.clear();

    for (const auto &artist: get_artists())
    {
        std::vector<wstring> columns;

        wstring total_albums_str = L"";
        bool is_followed = false;
        
        if (auto api = api_proxy.lock())
        {
            auto albums = api->get_artist_albums(artist.id, get_album_types_filters());

            auto albums_count = albums->peek_total();
            total_albums_str = albums_count > 0 ? std::to_wstring(albums_count) : L"";
            is_followed = api->get_library()->is_artist_followed(artist.id);
        }

        // column C0 - followers count
        columns.push_back(utils::format(L"{: >9}", format_followers(artist.followers_total)));

        // column C1 - popularity
        columns.push_back(utils::format(L"{:5}", artist.popularity));

        // column C2 - main genre
        columns.push_back(artist.get_main_genre());
        
        // column C3 - total albums
        columns.push_back(utils::format(L"{: >6}", total_albums_str));

        // column C4 - is saved in collection status
        columns.push_back(is_followed ? L" + " : L"");
        
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
        api_proxy.lock()->get_artist_albums(data->id, get_album_types_filters())->get_total();
        return true;
    }
    return false;
}

const sort_modes_t& artists_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarName),       SM_NAME,            { VK_F3, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarFollow),     SM_SIZE,            { VK_F4, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarPopularity), SM_COMPRESSEDSIZE,  { VK_F5, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

intptr_t artists_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const artist_t*>(data1),
        &item2 = static_cast<const artist_t*>(data2);

    #if defined (__clang__)
    #   pragma clang diagnostic ignored "-Wswitch"
    #endif
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

const panel_modes_t* artists_base_view::get_panel_modes() const
{
    static const panel_mode_t::column_t
        Followers   { L"C0",    get_text(MSortColFollow),       L"9" },
        Popularity  { L"C1",    get_text(MSortColPopularity),   L"5" },
        MainGenre   { L"C2",    get_text(MSortColGenre),        L"25" },
        AlbumsCount { L"C3",    get_text(MSortColAlbumsCount),  L"6" },
        IsFollowed  { L"C4",    get_text(MSortColSaved),        L"3" },
        Name        { L"NON",   get_text(MSortColName),         L"0" },
        NameFixed   { L"NON",   get_text(MSortColName),         L"30" },
        Genres      { L"Z",     get_text(MSortColGenre),        L"0" };

    static panel_modes_t modes{
        /* 0 */ PM::dummy(8),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ &Name, &IsFollowed, &AlbumsCount, &Followers, &Popularity }),
        /* 4 */ PM({ &Name, &IsFollowed, &AlbumsCount, &MainGenre }),
        /* 5 */ PM({ &Name, &IsFollowed, &AlbumsCount, &Followers, &Popularity, &MainGenre }, true),
        /* 6 */ PM({ &NameFixed, &Genres }),
        /* 7 */ PM({ &Name, &IsFollowed, &AlbumsCount, &Followers, &Popularity, &Genres }, true),
        /* 5 */ PM({ &Name, &IsFollowed, &AlbumsCount, &Followers, &Popularity, &MainGenre }),
        /* 9 */ PM::dummy(8),
    };
    
    return &modes;
}

intptr_t artists_base_view::process_key_input(int combined_key)
{
    using namespace utils::keys;

    switch (combined_key)
    {
        // starting playbackl for the target artist
        case VK_F4:
        {
            auto item = utils::far3::panels::get_current_item(get_panel_handle());
            if (auto api = api_proxy.lock(); item && api)
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    // no several artists multiselection is allowed, starting playback for
                    // the one which is currently under cursor
                    api->start_playback(artist_t::make_uri(user_data->id));
                    return TRUE;
                }
            }
            return TRUE;
        }

        // follow/unfollow
        case VK_F8:
        {
            const auto &ids = get_selected_items();

            if (auto api = api_proxy.lock(); api && !ids.empty())
            {
                // what to do - like or unlike - with the whole list of items
                // we decide based on the first item state
                if (auto *library = api->get_library(); library->is_artist_followed(ids[0], true))
                {
                    library->unfollow_artists(ids);
                }
                else
                {
                    library->follow_artists(ids);
                }
            }
            return TRUE;
        }

        // redirect to Spotify Web
        case VK_RETURN + mods::shift:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (const artist_t *artist = static_cast<const artist_t*>(user_data); !artist->urls.spotify.empty())
                        utils::open_web_browser(artist->urls.spotify);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

const key_bar_info_t* artists_base_view::get_key_bar_info()
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
        if (auto *user_data = unpack_user_data(item->UserData); user_data && user_data->is_valid())
        {
            if (api->get_library()->is_artist_followed(user_data->id))
                key_bar[{ VK_F8, 0 }] = get_text(MUnlike);
            else
                key_bar[{ VK_F8, 0 }] = get_text(MLike);
        }
    }

    return &key_bar;
}

void artists_base_view::on_artists_statuses_changed(const item_ids_t &ids)
{
    on_artists_statuses_received(ids);
}

void artists_base_view::on_artists_statuses_received(const item_ids_t &ids)
{
    std::unordered_set<item_id_t> unique_ids(ids.begin(), ids.end());

    const auto &it = std::find_if(items.begin(), items.end(),
        [&unique_ids](item_t &item) { return unique_ids.contains(item.id); });

    // if any of view's tracks are changed, we need to refresh the panel
    if (it != items.end())
        events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
followed_artists_view::followed_artists_view(HANDLE panel, api_weak_ptr_t api_proxy):
    artists_base_view(panel, api_proxy, get_text(MPanelArtists), get_text(MPanelCollection))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_library()->get_followed_artists();
        collection->fetch();
    }
}

config::settings::view_t followed_artists_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F3
    return { 0, false, 3 };
}

std::generator<const artist_t&> followed_artists_view::get_artists()
{
    if (collection)
        for (const auto &a: *collection) co_yield a;
}

void followed_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist(api_proxy, artist);
}

void followed_artists_view::show_filters_dialog()
{
    // unfinished, not for the aplha release

    /*PluginDialogBuilder builder(config::ps_info, guids::MainGuid, FarMessageGuid, L"Test Dialog", NULL);

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
    artists_base_view(panel, api, get_text(MPanelArtists), get_text(MPanelRecents))
{
    rebuild_items();
    utils::events::start_listening<play_history_observer>(this);
}

recent_artists_view::~recent_artists_view()
{
    utils::events::stop_listening<play_history_observer>(this);
    items.clear();
}

config::settings::view_t recent_artists_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F4 (with main genre)
    return { 0, false, 4 };
}

const sort_modes_t& recent_artists_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = artists_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarPlayedAt), SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
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
        if (it->artists.size() > 0)
            recent_artists[it->get_artist().id] = *it;

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
    for (const auto &i: items) co_yield i;
}

void recent_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist(api_proxy, artist, mv_build_t::albums_idx,
        [api = api_proxy] { events::show_recents(api, mv_build_t::artists_idx); });
}

void recent_artists_view::on_history_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
user_top_artists_view::user_top_artists_view(HANDLE panel, api_weak_ptr_t api):
    artists_base_view(panel, api, get_text(MPanelUserTopArtists), get_text(MPanelUserTopItems))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_user_top_artists();
        collection->fetch(false, false, 4);
    }
}

config::settings::view_t user_top_artists_view::get_default_settings() const
{
    // sort mode - by Name; ascending; view mode - F4 (with main genre)
    return { 0, false, 4 };
}

const sort_modes_t& user_top_artists_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarUnsorted), SM_UNSORTED, { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

std::generator<const artist_t&> user_top_artists_view::get_artists()
{
    if (collection)
        for (const auto &a: *collection) co_yield a;
}

void user_top_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist(api_proxy, artist, mv_build_t::default_idx,
        [api = api_proxy] { events::show_user_top_items(api); });
}


} // namespace ui
} // namespace spotifar