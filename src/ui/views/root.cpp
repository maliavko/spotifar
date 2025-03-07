#include "root.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    artists_view_id = "artists",
    playlists_view_id = "playlists",
    recents_view_id = "recents";

root_view::root_view(api_abstract *api):
    view("root_view"),
    api_proxy(api)
{
}

const wchar_t* root_view::get_dir_name() const
{
    // should be empty, so Far closes plugin in case of hitting ".."
    static const wchar_t *cur_dir = { L"" };
    return cur_dir;
}

const wchar_t* root_view::get_title() const
{
    static const wchar_t *title = { L"Root Menu" };
    return title;
}

const view::key_bar_info_t* root_view::get_key_bar_info()
{
    // TODO: test data
    static key_bar_info_t key_bar{
        { { VK_F4, 0 }, get_text(MKeyBarF4) },
    };

    return &key_bar;
}

const view::info_lines_t* root_view::get_info_lines()
{
    // TODO: test data
    static info_lines_t lines{
        { L"1", L"1" },
        { L"3", L"3", IPLFLAGS_SEPARATOR },
        { L"4", L"4" },
    };
    return &lines;
}

void root_view::update_panel_info(OpenPanelInfo *info)
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

const view::items_t* root_view::get_items()
{
    static items_t items; items.clear();

    {
        items.push_back(pack_menu_item(
            artists_view_id,
            MPanelArtistsItemLabel,
            MPanelArtistsItemDescr,
            followed_artists_requester()
        ));
    }

    {
        items.push_back(pack_menu_item(
            playlists_view_id,
            MPanelPlaylistsItemLabel,
            MPanelPlaylistsItemDescr,
            user_playlists_requester()
        ));
    }

    {
        items.push_back({
            recents_view_id,
            get_text(MPanelRecentsItemLabel),
            get_text(MPanelRecentsItemDescr),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
        });
    }

    return &items;
}

intptr_t root_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
    {
        return FALSE;
    }

    if (data->id == artists_view_id)
    {
        ui::events::show_artists_view();
        return TRUE;
    }
    
    if (data->id == playlists_view_id)
    {
        ui::events::show_playlists_view();
        return TRUE;
    }
    
    if (data->id == recents_view_id)
    {
        ui::events::show_recents_view();
        return TRUE;
    }

    return FALSE;
}

bool root_view::request_extra_info(const user_data_t* data)
{
    if (data->id == artists_view_id)
        return followed_artists_requester()(api_proxy);

    if (data->id == playlists_view_id)
        return user_playlists_requester()(api_proxy);

    // TODO: recents?
    return false;
}

} // namespace ui
} // namespace spotifar