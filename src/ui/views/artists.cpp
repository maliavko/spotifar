#include "artists.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

artists_base_view::artists_base_view(api_abstract *api, const string &view_uid,
    return_callback_t callback):
    view(view_uid, callback), api_proxy(api)
    {}

void artists_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static wstring column_name;
    
    column_name = L"Name";

    static const wchar_t* titles_3[] = { column_name.c_str(), L"Albums", L"Followers", L"Pop %" };
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

    static const wchar_t* titles_5[] = { column_name.c_str(), L"Albums", L"Followers", L"Pop %", L"Genre" };
    modes[5].ColumnTypes = L"NON,C3,C0,C1,C2";
    modes[5].ColumnWidths = L"0,6,9,5,25";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { column_name.c_str(), L"Genres" };
    modes[6].ColumnTitles = titles_6;
    modes[6].StatusColumnTypes = NULL;
    modes[6].StatusColumnWidths = NULL;

    static const wchar_t* titles_7[] = { column_name.c_str(), L"Albums", L"Genres" };
    modes[7].ColumnTitles = titles_7;
    modes[7].StatusColumnTypes = NULL;
    modes[7].StatusColumnWidths = NULL;

    modes[8] = modes[5]; // the same as 5th, but not fullscreen
    modes[8].Flags &= ~PMFLAGS_FULLSCREEN;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view::items_t* artists_base_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &a: get_artists())
    {
        std::vector<wstring> column_data;

        // column C0 - followers count
        if (a.followers_total < 1000000)
            column_data.push_back(std::format(L"{:9}", a.followers_total));
        else if (a.followers_total < 1000000000)
            column_data.push_back(std::format(L"{:7.2f} M", a.followers_total / 1000000.0));
        else if (a.followers_total < 1000000000000)
            column_data.push_back(std::format(L"{:7.2f} B", a.followers_total / 1000000000.0));

        // column C1 - popularity
        column_data.push_back(std::format(L"{:5}", a.popularity));

        // column C2 - first (main?) genre
        column_data.push_back(a.genres.size() > 0 ? utils::to_wstring(a.genres[0]) : L"");
        
        // column C3 - total albums
        auto albums = api_proxy->get_artist_albums(a.id);
        auto total_albums = albums->peek_total();
    
        wstring albums_count = L"";
        if (total_albums > 0)
            albums_count = std::format(L"{: >6}", total_albums);
        column_data.push_back(albums_count);

        items.push_back({
            a.id,
            a.name,
            utils::to_wstring(utils::string_join(a.genres, ", ")),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            column_data,
            const_cast<artist_t*>(&a)
        });
    }

    return &items;
}

intptr_t artists_base_view::select_item(const data_item_t *data)
{
    const auto *artist = static_cast<const artist_t*>(data);
    if (artist != nullptr)
    {
        show_albums_view(*artist);
        return TRUE;
    }
    return FALSE;
}

bool artists_base_view::request_extra_info(const data_item_t *data)
{
    if (data != nullptr)
    {
        api_proxy->get_artist_albums(data->id)->get_total();
        return true;
    }
    return false;
}

const view::sort_modes_t& artists_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Followers",     SM_SIZE,    VK_F4 + mods::ctrl },
        { L"Popularity",    SM_OWNER,   VK_F5 + mods::ctrl },
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

        case SM_OWNER:
            return item1->popularity - item2->popularity;

        case SM_SIZE:
            return item1->followers_total - item2->followers_total;
    }
    return -2;
}


followed_artists_view::followed_artists_view(api_abstract *api):
    artists_base_view(api, "followed_artists_view", std::bind(events::show_collections, api)),
    collection(api_proxy->get_followed_artists())
{
}

const wstring& followed_artists_view::get_dir_name() const
{
    static wstring dir_name(get_text(MPanelArtistsItemLabel));
    return dir_name;
}

config::settings::view_t followed_artists_view::get_default_settings() const
{
    return { 0, false, 3 };
}

std::generator<const artist_t&> followed_artists_view::get_artists()
{
    if (collection->fetch())
        for (const auto &a: *collection)
            co_yield a;
}

void followed_artists_view::show_albums_view(const artist_t &artist) const
{
    events::show_artist_albums(api_proxy, artist,
        std::bind(events::show_artists_collection, api_proxy));
}


recent_artists_view::recent_artists_view(api_abstract *api):
    artists_base_view(api, "recent_artists_view", std::bind(events::show_recents, api))
{
    utils::events::start_listening<play_history_observer>(this);

    rebuild_items();
}

recent_artists_view::~recent_artists_view()
{
    utils::events::stop_listening<play_history_observer>(this);

    items.clear();
}

const wstring& recent_artists_view::get_dir_name() const
{
    static wstring dir_name(get_text(MPanelArtistsItemLabel));
    return dir_name;
}

config::settings::view_t recent_artists_view::get_default_settings() const
{
    return { 1, false, 6 };
}

const view::sort_modes_t& recent_artists_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = artists_base_view::get_sort_modes();
        modes.push_back({ L"Played at", SM_MTIME, VK_F6 + mods::ctrl });
    }
    return modes;
}

void recent_artists_view::rebuild_items()
{
    items.clear();

    std::unordered_map<string, history_item_t> recent_artists;
    
    // collecting all the tracks' artists listened to
    for (const auto &item: api_proxy->get_play_history())
        if (item.track.artists.size() > 0)
            recent_artists[item.track.artists[0].id] = item;

    if (recent_artists.size() > 0)
    {
        const auto &keys = std::views::keys(recent_artists);
        const auto &ids = item_ids_t(keys.begin(), keys.end());

        for (const auto &artist: api_proxy->get_artists(ids))
            items.push_back(history_artist_t(recent_artists[artist.id].played_at, artist));
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
        std::bind(events::show_recent_artists, api_proxy));
}

void recent_artists_view::on_items_changed()
{
    rebuild_items();
    
    panels::update(PANEL_ACTIVE);
    panels::redraw(PANEL_ACTIVE);
}

} // namespace ui
} // namespace spotifar