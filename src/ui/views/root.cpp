#include "root.hpp"
#include "ui/events.hpp"
#include "ui/views/artists.hpp"
#include "ui/views/playlists.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    artists_view_id = "artists",
    playlists_view_id = "playlists",
    recents_view_id = "recents";

root_view::root_view(api_abstract *api):
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
        { { VK_F4, 0 }, utils::far3::get_text(MKeyBarF4) },
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
    static const wchar_t* titles[] = { L"Name", L"Size" };

    static PanelMode modes[10];

    modes[3].ColumnTypes = L"NON,ST";
    modes[3].ColumnWidths = L"0,6";
    modes[3].ColumnTitles = titles;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    modes[4] = modes[3];

    modes[5] = modes[3];
    modes[5].Flags = PMFLAGS_FULLSCREEN;
    
    modes[8] = modes[3];

    modes[9] = modes[3];

    modes[0].ColumnTypes = L"NON,STC";
    modes[0].ColumnWidths = L"0,6";
    modes[0].ColumnTitles = titles;
    modes[0].StatusColumnTypes = NULL;
    modes[0].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}


const view::items_t* root_view::get_items() const
{
    static items_t items{
        {
            artists_view_id,
            get_text(MPanelArtistsItemLabel),
            get_text(MPanelArtistsItemDescr),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
        },
        {
            playlists_view_id,
            get_text(MPanelPlaylistsItemLabel),
            get_text(MPanelPlaylistsItemDescr),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
        },
        {
            recents_view_id,
            get_text(MPanelRecentsItemLabel),
            get_text(MPanelRecentsItemDescr),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
        }
    };
    return &items;
}

intptr_t root_view::select_item(const string &view_id)
{
    if (view_id == artists_view_id)
    {
        ui::events::show_artists_view();
        return TRUE;
    }
    
    if (view_id == playlists_view_id)
    {
        ui::events::show_playlists_view();
        return TRUE;
    }
    
    if (view_id == recents_view_id)
    {
        ui::events::show_recents_view();
        return TRUE;
    }

    return FALSE;
}

std::shared_ptr<view::find_processor> root_view::get_find_processor(const string &view_id)
{
    if (view_id == artists_view_id)
        return std::make_shared<artists_view::find_processor>(api_proxy);

    if (view_id == playlists_view_id)
        return std::make_shared<playlists_view::find_processor>(api_proxy);

    // TODO: recents?
    
    return nullptr;
}

} // namespace ui
} // namespace spotifar