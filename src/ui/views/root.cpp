#include "root.hpp"
#include "ui/events.hpp"

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

auto root_view::get_key_bar_info() -> const key_bar_info_t*
{
    // TODO: test data
    static key_bar_info_t key_bar{
        { { VK_F4, 0 }, utils::far3::get_text(MKeyBarF4) },
    };

    return &key_bar;
}

auto root_view::get_info_lines() -> const info_lines_t*
{
    // TODO: test data
    static info_lines_t lines{
        { L"1", L"1" },
        { L"3", L"3", IPLFLAGS_SEPARATOR },
        { L"4", L"4" },
    };
    return &lines;
}

auto root_view::get_items() -> const items_t*
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

} // namespace ui
} // namespace spotifar