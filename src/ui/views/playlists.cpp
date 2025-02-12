#include "playlists.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlists_view::playlists_view(spotify::api *api):
    view(get_text(MPanelPlaylistsItemLabel)),
    api(api)
{
}

view::view_items_t playlists_view::get_items()
{
    view_items_t result;
    for (const auto &p: api->get_library().get_playlists())
        result.push_back({p.id, p.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

intptr_t playlists_view::select_item(const string &playlist_id)
{
    if (playlist_id.empty())
    {
        events::show_root_view();
        return TRUE;
    }
    
    auto playlist = api->get_library().get_playlist(playlist_id);
    if (playlist.is_valid())
    {
        events::show_playlist_view(playlist);
        return TRUE;
    }

    return FALSE;
}

} // namespace ui
} // namespace spotifar