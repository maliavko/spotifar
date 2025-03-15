#include "collection.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    artists_view_id = "artists",
    albums_view_id = "albums",
    tracks_view_id = "tracks",
    playlists_view_id = "playlists";

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
    static items_t items;
    
    items.assign({
        pack_menu_item(
            artists_view_id,
            MPanelArtistsItemLabel, MPanelArtistsItemDescr,
            followed_artists_requester()
        ),
        pack_menu_item(
            albums_view_id,
            MPanelAlbumsItemLabel, MPanelAlbumsItemDescr,
            saved_albums_requester()
        ),
        pack_menu_item(
            tracks_view_id,
            MPanelTracksItemLabel, MPanelTracksItemDescr,
            saved_tracks_requester()
        ),
        pack_menu_item(
            playlists_view_id,
            MPanelPlaylistsItemLabel, MPanelPlaylistsItemDescr,
            user_playlists_requester()
        ),
    });

    return &items;
}

intptr_t collection_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
    {
        ui::events::show_root_view(api_proxy);
        return TRUE;
    }

    if (data->id == artists_view_id)
    {
        ui::events::show_artists_collection_view(api_proxy);
        return TRUE;
    }

    if (data->id == albums_view_id)
    {
        ui::events::show_albums_collection_view(api_proxy);
        return TRUE;
    }

    if (data->id == tracks_view_id)
    {
        ui::events::show_tracks_collection_view(api_proxy);
        return TRUE;
    }
    
    if (data->id == playlists_view_id)
    {
        ui::events::show_playlists_view(api_proxy);
        return TRUE;
    }

    return FALSE;
}

bool collection_view::request_extra_info(const user_data_t* data)
{
    if (data->id == artists_view_id)
        return followed_artists_requester()(api_proxy);

    if (data->id == albums_view_id)
        return saved_albums_requester()(api_proxy);

    if (data->id == tracks_view_id)
        return saved_tracks_requester()(api_proxy);

    if (data->id == playlists_view_id)
        return user_playlists_requester()(api_proxy);

    return false;
}

} // namespace ui
} // namespace spotifar