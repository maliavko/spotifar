#include "artists.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using mv_build_t = multiview_builder_t;
using PM = view::panel_mode_t;
using utils::far3::get_text;
using namespace spotify;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
const view::items_t& artists_base_view::get_items()
{
    static view::items_t items; items.clear();

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

const view::sort_modes_t& artists_base_view::get_sort_modes() const
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

const view::panel_modes_t* artists_base_view::get_panel_modes() const
{
    static const view::panel_mode_t::column_t
        Followers   { L"C0",    get_text(MSortColFollow),       L"9" },
        Popularity  { L"C1",    get_text(MSortColPopularity),   L"5" },
        MainGenre   { L"C2",    get_text(MSortColGenre),        L"25" },
        AlbumsCount { L"C3",    get_text(MSortColAlbumsCount),  L"6" },
        Name        { L"NON",   get_text(MSortColName),         L"0" },
        NameFixed   { L"NON",   get_text(MSortColName),         L"30" },
        Genres      { L"Z",     get_text(MSortColGenre),        L"0" };

    static panel_modes_t modes{
        /* 0 */ PM::dummy(8),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ &Name, &AlbumsCount, &Followers, &Popularity }),
        /* 4 */ PM({ &Name, &AlbumsCount }),
        /* 5 */ PM({ &Name, &AlbumsCount, &Followers, &Popularity, &MainGenre }, true),
        /* 6 */ PM({ &NameFixed, &Genres }),
        /* 7 */ PM({ &NameFixed, &AlbumsCount, &Genres }, true),
        /* 8 */ PM::dummy(8, true),
        /* 9 */ PM::dummy(8),
    };
    
    return &modes;
}

intptr_t artists_base_view::process_key_input(int combined_key)
{
    using namespace utils::keys;

    switch (combined_key)
    {
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
            
            log::global->error("There is an error occured while getting a current panel item");
            return TRUE;
        }
        case VK_F8:
        {
            /*const auto &ids = get_selected_items();

            if (auto api = api_proxy.lock(); !ids.empty())
                // what to do - like or unlike - with the whole list of items
                // we decide based on the first item state
                if (api->is_album_saved(ids[0], true))
                    api->remove_saved_albums(ids);
                else
                    api->save_albums(ids);*/

            return TRUE;
        }
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


//-----------------------------------------------------------------------------------------------------------
followed_artists_view::followed_artists_view(HANDLE panel, api_weak_ptr_t api_proxy):
    artists_base_view(panel, api_proxy, get_text(MPanelArtists), get_text(MPanelCollection))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_followed_artists();
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
    return { 1, false, 6 };
}

const view::sort_modes_t& recent_artists_view::get_sort_modes() const
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
        collection->fetch(false, true, 4);
    }
}

config::settings::view_t user_top_artists_view::get_default_settings() const
{
    return { 1, false, 1 };
}

const view::sort_modes_t& user_top_artists_view::get_sort_modes() const
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