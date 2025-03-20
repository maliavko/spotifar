#include "recents.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    tracks_id = "tracks",
    artists_id = "artists",
    albums_id = "albums",
    playlists_id = "playlists";

struct root_data_t: public data_item_t
{
    int name_key, descr_key;
};

static std::vector<root_data_t> menu_items{
    { { tracks_id }, MPanelTracksItemLabel, MPanelTracksItemDescr },
    { { artists_id }, MPanelArtistsItemLabel, MPanelArtistsItemDescr },
    { { albums_id }, MPanelAlbumsItemLabel, MPanelAlbumsItemDescr },
    { { playlists_id }, MPanelPlaylistsItemLabel, MPanelPlaylistsItemDescr },
};

recents_view::recents_view(api_abstract *api):
    view("recents_view", std::bind(events::show_root, api)),
    api_proxy(api)
{
}

const wstring& recents_view::get_dir_name() const
{
    static wstring cur_dir(get_text(MPanelRecentsItemLabel));
    return cur_dir;
}

const view::sort_modes_t& recents_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,        VK_F3 + mods::ctrl },
        { L"Unsorted",  SM_UNSORTED,    VK_F7 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t recents_view::get_default_settings() const
{
    return { 1, false, 3 };
}

void recents_view::update_panel_info(OpenPanelInfo *info)
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
    info->PanelModesNumber = std::size(modes);
}

const view::items_t* recents_view::get_items()
{
    static items_t items; items.clear();
    
    for (auto &item: menu_items)
    {
        items.push_back({
            item.id,
            get_text(item.name_key), get_text(item.descr_key),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL, {},
            &item
        });
    }

    return &items;
}

intptr_t recents_view::select_item(const data_item_t *data)
{
    if (data->id == tracks_id)
    {
        ui::events::show_recent_tracks(api_proxy);
        return TRUE;
    }
    
    if (data->id == albums_id)
    {
        ui::events::show_recent_albums(api_proxy);
        return TRUE;
    }
    
    // if (data->id == recents_id)
    // {
    //     ui::events::show_recents(api_proxy);
    //     return TRUE;
    // }

    return FALSE;
}

} // namespace ui
} // namespace spotifar