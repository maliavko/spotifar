#include "collection.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    artists_id = "artists",
    albums_id = "albums",
    tracks_id = "tracks",
    playlists_id = "playlists";

struct collection_data_t: public spotify::data_item
{
    int name_key, descr_key;
};

static std::vector<collection_data_t> menu_items{
    { { artists_id }, MPanelArtistsItemLabel, MPanelArtistsItemDescr },
    { { albums_id }, MPanelAlbumsItemLabel, MPanelAlbumsItemDescr },
    { { tracks_id }, MPanelTracksItemLabel, MPanelTracksItemDescr },
    { { playlists_id }, MPanelPlaylistsItemLabel, MPanelPlaylistsItemDescr },
};

template<class R>
size_t calculate_menu_total(api_abstract *api, bool only_from_cache)
{
    auto req = R();
    if (only_from_cache && !api->is_request_cached(req.get_url()))
        return 0;

    if (req(api))
        return req.get_total();

    return 0;
}

size_t calculate_totals(api_abstract *api, const string &menu_id, bool only_from_cache = false)
{
    if (menu_id == artists_id)
        return calculate_menu_total<followed_artists_requester>(api, only_from_cache);

    if (menu_id == albums_id)
        return calculate_menu_total<saved_albums_requester>(api, only_from_cache);

    if (menu_id == tracks_id)
        return calculate_menu_total<saved_tracks_requester>(api, only_from_cache);

    if (menu_id == playlists_id)
        return calculate_menu_total<user_playlists_requester>(api, only_from_cache);
    
    return 0;
}

collection_view::collection_view(api_abstract *api):
    view("collection_view"),
    api_proxy(api)
{
}

const wstring& collection_view::get_dir_name() const
{
    static const wstring dir_name(L"Collection");
    return dir_name;
}

const view::sort_modes_t& collection_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,        VK_F3 + mods::ctrl },
        { L"Unsorted",  SM_UNSORTED,    VK_F7 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t collection_view::get_default_settings() const
{
    return { 0, false, 3 };
}

void collection_view::update_panel_info(OpenPanelInfo *info)
{
    static const wchar_t* titles[] = { L"Name", L"Count" };

    static PanelMode modes[10];

    modes[3].ColumnTypes = L"NON,C0";
    modes[3].ColumnWidths = L"0,6";
    modes[3].ColumnTitles = titles;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    modes[4] = modes[3];

    modes[5] = modes[3];
    modes[5].Flags = PMFLAGS_FULLSCREEN;
    
    modes[8] = modes[3];

    modes[9] = modes[3];

    modes[0].ColumnTypes = L"NON,C0";
    modes[0].ColumnWidths = L"0,6";
    modes[0].ColumnTitles = titles;
    modes[0].StatusColumnTypes = NULL;
    modes[0].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

const view::items_t* collection_view::get_items()
{
    static items_t items; items.clear();

    for (auto &item: menu_items)
    {
        auto totals = calculate_totals(api_proxy, item.id, true);

        wstring entries_count = L"";
        if (totals > 0)
            entries_count = std::format(L"{: >6}", totals);

        items.push_back({
            item.id, get_text(item.name_key), get_text(item.descr_key),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            {
                entries_count
            },
            &item
        });
    }

    return &items;
}

intptr_t collection_view::select_item(const spotify::data_item* data)
{
    if (data == nullptr)
    {
        ui::events::show_root_view(api_proxy);
        return TRUE;
    }

    if (data->id == artists_id)
    {
        ui::events::show_artists_collection_view(api_proxy);
        return TRUE;
    }

    if (data->id == albums_id)
    {
        ui::events::show_albums_collection_view(api_proxy);
        return TRUE;
    }

    if (data->id == tracks_id)
    {
        ui::events::show_tracks_collection_view(api_proxy);
        return TRUE;
    }
    
    if (data->id == playlists_id)
    {
        ui::events::show_playlists_view(api_proxy);
        return TRUE;
    }

    return FALSE;
}

bool collection_view::request_extra_info(const spotify::data_item* data)
{
    return calculate_totals(api_proxy, data->id) > 0;
}

} // namespace ui
} // namespace spotifar