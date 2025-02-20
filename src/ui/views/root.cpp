#include "root.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    artists_view_id = "artists",
    playlists_view_id = "playlists";

root_view::root_view(spotify::api_abstract *api):
    view(get_text(MPanelRootItemLabel)),
    api_proxy(api)
{
}

view::view_items_t root_view::get_items()
{
    return view_items_t{
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
        }
    };
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

    return FALSE;
}

} // namespace ui
} // namespace spotifar