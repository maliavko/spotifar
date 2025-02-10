#include "root.hpp"
#include "artists.hpp"
#include "playlists.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    artist_view_id = "artists",
    playlists_view_id = "playlists";

root_view::root_view(spotify::api *api):
    view(get_text(MPanelRootItemLabel)),
    api(api)
{
}

view::view_items_t root_view::get_items()
{
    return view_items_t{
        {
            artist_view_id,
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

std::shared_ptr<view> root_view::select_item(const string &view_id)
{
    if (view_id == artist_view_id)
        return artists_view::build(api);
    
    if (view_id == playlists_view_id)
        return playlists_view::build(api);

    return NULL;
}

std::shared_ptr<root_view> root_view::build(spotify::api *api)
{
    return std::make_shared<root_view>(api);
}

} // namespace ui
} // namespace spotifar