#include "albums.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

/// @brief Helper-method for adjusting columns data prepared by views for Far API.
/// Extract columns data and add a new column to the front
/// @tparam ViewType 
/// @param modes filled Far API struct
/// @param col_name new column name
/// @param col_size new column size
/// @param col_uid new column id (C1,2,....)
template<class ViewType>
static void update_columns_info(PanelMode *modes, const wchar_t *col_name, size_t col_size, const wchar_t *col_uid)
{
    static struct
    {
        std::vector<const wchar_t*> titles;
        wstring width, types;
    } columns[9];
    
    for (size_t idx = 3; idx < 9; idx++)
    {
        size_t columns_count = utils::string_split(modes[idx].ColumnWidths, L',').size();
        assert (columns_count > 0);

        // extending all the columns data with the track number column
        columns[idx].titles.push_back(col_name);
        columns[idx].titles.insert(columns[idx].titles.end(),
            modes[idx].ColumnTitles, modes[idx].ColumnTitles + columns_count);

        columns[idx].width = std::format(L"{},{}", col_size, modes[idx].ColumnWidths);

        columns[idx].types = std::format(L"{},{}", col_uid, modes[idx].ColumnTypes);

        // overriding the panel modes with the new columns data
        modes[idx].ColumnTitles = &columns[idx].titles[0];
        modes[idx].ColumnWidths = columns[idx].width.c_str();
        modes[idx].ColumnTypes = columns[idx].types.c_str();
    }

    modes[9] = modes[0] = modes[8];
}

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

const view::items_t& albums_base_view::get_items()
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

            is_saved = api->is_album_saved(album.id);
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
        
        const auto &copyright = album.get_main_copyright();

        items.push_back({
            album.id,
            album.name,
            copyright.text,
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<album_t*>(&album)
        });
    }
    return items;
}

void albums_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Yr", L"Name", L"[+]", L"Tx", L"Length", L"Type", L"Pop %" };
    modes[3].ColumnTypes = L"C0,NON,C7,C2,C4,C1,C5";
    modes[3].ColumnWidths = L"6,0,3,4,8,6,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Yr", L"[+]", L"Name", L"Artist"};
    modes[4].ColumnTypes = L"C0,C7,NON,C6";
    modes[4].ColumnWidths = L"6,3,0,0";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Yr", L"Name", L"Artist", L"[+]", L"Tx", L"Length", L"Type", L"Pop %", L"Copyrights" };
    modes[5].ColumnTypes = L"C0,NON,C6,C7,C2,C4,C1,C5,Z";
    modes[5].ColumnWidths = L"6,30,30,3,4,8,6,5,0";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { L"Yr", L"Name", L"[+]", L"Copyrights" };
    modes[6].ColumnTypes = L"C0,NON,C7,Z";
    modes[6].ColumnWidths = L"6,0,3,0";
    modes[6].ColumnTitles = titles_6;

    static const wchar_t* titles_7[] = { L"Yr", L"Name", L"Artist", L"[+]", L"Copyrights" };
    modes[7].ColumnTypes = L"C0,NON,C6,C7,Z";
    modes[7].ColumnWidths = L"6,30,30,3,0";
    modes[7].ColumnTitles = titles_7;
    modes[7].StatusColumnTypes = NULL;
    modes[7].StatusColumnWidths = NULL;
    modes[7].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_8[] = { L"Yr", L"Name", L"Artist", L"[+]", L"Tx", L"Length", L"Type", L"Pop %" };
    modes[8].ColumnTypes = L"C0,NON,C6,C7,C2,C4,C1,C5";
    modes[8].ColumnWidths = L"6,30,0,3,4,8,6,5";
    modes[8].ColumnTitles = titles_8;
    modes[8].StatusColumnTypes = NULL;
    modes[8].StatusColumnWidths = NULL;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view::sort_modes_t& albums_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,            { VK_F3, LEFT_CTRL_PRESSED } },
        { L"Release date",  SM_ATIME,           { VK_F4, LEFT_CTRL_PRESSED } },
        { L"Popularity",    SM_COMPRESSEDSIZE,  { VK_F5, LEFT_CTRL_PRESSED } },
        { L"Tracks",        SM_SIZE,            { VK_F6, LEFT_CTRL_PRESSED } },
        { L"Artist name",   SM_OWNER,           { VK_F7, LEFT_CTRL_PRESSED } },
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
    switch (combined_key)
    {
        case VK_F4:
        {
            auto item = utils::far3::panels::get_current_item(get_panel_handle());
            if (auto api = api_proxy.lock(); item && api)
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    log::global->info("Starting playback from the panel, {}", user_data->id);
                    api->start_playback(album_t::make_uri(user_data->id));

                    return TRUE;
                }
            }
            
            log::global->error("There is an error occured while getting a current panel item");
            return TRUE;
        }
        case VK_F8:
        {
            const auto &ids = get_selected_items();

            if (auto api = api_proxy.lock(); !ids.empty())
                // what to do - like or unlike - with the whole list of items
                // we decide based on the first item state
                if (api->is_album_saved(ids[0], true))
                    api->remove_saved_albums(ids);
                else
                    api->save_albums(ids);

            return TRUE;
        }
    }
    return FALSE;
}

const view::key_bar_info_t* albums_base_view::get_key_bar_info()
{
    static key_bar_info_t key_bar{};

    auto view_crc32 = get_crc32();
    auto item = utils::far3::panels::get_current_item(get_panel_handle());

    // right after switching the directory on the panel, when we trap here `panels::get_current_item`
    // returns the `item` from  the previous directory. Checking crc32 helps to identify
    // when the panel is refreshed eventually and we can be sure the item is valid
    if (item->CRC32 != view_crc32) return nullptr;

    if (auto *user_data = unpack_user_data(item->UserData); auto api = api_proxy.lock())
    {
        if (api->is_album_saved(user_data->id))
            key_bar[{ VK_F8, 0 }] = get_text(MUnlike);
        else
            key_bar[{ VK_F8, 0 }] = get_text(MLike);
    }

    return &key_bar;
}

void albums_base_view::on_saved_albums_changed(const item_ids_t &ids)
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
    events::show_album_tracks(api_proxy, album,
        std::bind(events::show_artist, api_proxy, artist, get_return_callback()));
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
    albums_base_view(panel, api_proxy, get_text(MPanelAlbumsItemLabel), get_text(MPanelCollectionItemLabel))
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_albums();
}

config::settings::view_t saved_albums_view::get_default_settings() const
{
    // sort mode - by `Saved at`; descending; view mode - F6
    return { 5, true, 3 };
}

std::generator<const album_t&> saved_albums_view::get_albums()
{
    if (collection && collection->fetch())
        for (const auto &a: *collection)
            co_yield a;
}

const view::sort_modes_t& saved_albums_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, { VK_F8, LEFT_CTRL_PRESSED } });
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

void saved_albums_view::update_panel_info(OpenPanelInfo *info)
{
    albums_base_view::update_panel_info(info);
    
    auto *modes = const_cast<PanelMode*>(info->PanelModesArray);

    // adding `Saved at` column to some of the view modes
    static const wchar_t* titles_4[] = { L"Saved at", L"Yr", L"Name", L"Artist" };
    modes[4].ColumnTypes = L"C8,C0,NON,C6";
    modes[4].ColumnWidths = L"12,6,30,0";
    modes[4].ColumnTitles = titles_4;

    static const wchar_t* titles_5[] = { L"Saved at", L"Yr", L"Name", L"Artist", L"Tx", L"Length", L"Type",
        L"Pop %", L"Copyrights" };
    modes[5].ColumnTypes = L"C8,C0,NON,C6,C2,C4,C1,C5,Z";
    modes[5].ColumnWidths = L"12,6,30,30,4,8,6,5,0";
    modes[5].ColumnTitles = titles_5;

    static const wchar_t* titles_7[] = { L"Saved at", L"Yr", L"Name", L"Artist", L"Copyrights" };
    modes[7].ColumnTypes = L"C8,C0,NON,C6,Z";
    modes[7].ColumnWidths = L"12,6,30,30,0";
    modes[7].ColumnTitles = titles_7;
}

void saved_albums_view::show_tracks_view(const album_t &album) const
{
    events::show_album_tracks(api_proxy, album,
        std::bind(events::show_collection, api_proxy));
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


//-----------------------------------------------------------------------------------------------------------
new_releases_view::new_releases_view(HANDLE panel, api_weak_ptr_t api_proxy):
    albums_base_view(panel, api_proxy, get_text(MPanelNewReleasesItemLabel))
{
    utils::events::start_listening<releases_observer>(this);
    rebuild_items();
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
        events::show_album_tracks(api_proxy, album,
            std::bind(events::show_artist, api_proxy, artist, get_return_callback()));
    }
}

void new_releases_view::on_releases_sync_finished(const recent_releases_t releases)
{
    events::refresh_panel(get_panel_handle());
}

void new_releases_view::rebuild_items()
{
    recent_releases.clear();

    if (auto api = api_proxy.lock())
    {
        const auto &releases = api->get_recent_releases();
        
        item_ids_t releases_ids;
        std::transform(releases.begin(), releases.end(), back_inserter(releases_ids),
            [](const auto &r) { return r.id; });

        recent_releases = api->get_albums(releases_ids);
    }
}

//-----------------------------------------------------------------------------------------------------------
recent_albums_view::recent_albums_view(HANDLE panel, api_weak_ptr_t api):
    albums_base_view(panel, api, get_text(MPanelAlbumsItemLabel), get_text(MPanelRecentsItemLabel))
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

const view::sort_modes_t& recent_albums_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ L"Played at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

std::generator<const album_t&> recent_albums_view::get_albums()
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
    
    // we pick the earliest possible item among all the duplicates, for that
    // the reverse order is used
    auto play_history = api->get_play_history();
    for (auto it = play_history.rbegin(); it != play_history.rend(); ++it)
        recent_albums[it->track.album.id] = *it;

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
        events::show_album_tracks(api_proxy, album,
            std::bind(events::show_artist, api_proxy, artist, get_return_callback()));
    }
}

void recent_albums_view::on_items_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
recently_saved_albums_view::recently_saved_albums_view(HANDLE panel, api_weak_ptr_t api_proxy):
    albums_base_view(panel, api_proxy, get_text(MPanelAlbumsItemLabel))
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_albums();
}

config::settings::view_t recently_saved_albums_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const view::sort_modes_t& recently_saved_albums_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = albums_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
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
        std::bind(events::show_recently_saved, api_proxy));
}

std::generator<const album_t&> recently_saved_albums_view::get_albums()
{
    // requesting only three pages of the data
    if (collection->fetch(false, true, 3))
        for (const auto &a: *collection)
            co_yield a;
}

} // namespace ui
} // namespace spotifar