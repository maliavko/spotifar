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
    // TODO: tmp code
    view_items_t result;
    for (auto& [id, a]: api->get_playlists())
        result.push_back({id, a.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
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

    return FALSE;
}

} // namespace ui
} // namespace spotifar