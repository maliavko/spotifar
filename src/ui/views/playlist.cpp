#include "playlist.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlist_view::playlist_view(spotify::api_abstract *api, const spotify::playlist &p):
    view(p.user_display_name),
    playlist(p),
    api_proxy(api)
{
}

view::view_items_t playlist_view::get_items()
{
    view_items_t result;
    for (const auto &t: api_proxy->get_playlist_tracks(playlist.id))
        result.push_back({t.track.id, t.track.name, L"", FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

intptr_t playlist_view::select_item(const string &track_id)
{
    if (track_id.empty())
    {
        events::show_playlists_view();
        return TRUE;
    }
    
    // TODO: what to do here? start playing?
    // auto playlist = api->get_playlist(playlist_id);

    return FALSE;
}

} // namespace ui
} // namespace spotifar