#include "playlist.hpp"
#include "playlists.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlist_view::playlist_view(spotify::api *api, const spotify::playlist &p):
    view(p.user_display_name),
    playlist(p),
    api(api)
{
}

view::view_items_t playlist_view::get_items()
{
    view_items_t result;
    for (const auto &t: api->get_library().get_playlist_tracks(playlist.id))
        result.push_back({t.track.id, t.track.name, L"", FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

std::shared_ptr<view> playlist_view::select_item(const string &track_id)
{
    if (track_id.empty())
        return playlists_view::build(api);
    
    // TODO: what to do here? start playing?
    // auto playlist = api->get_library().get_playlist(playlist_id);

    return nullptr;
}

std::shared_ptr<playlist_view> playlist_view::build(spotify::api *api, const spotify::playlist &p)
{
    return std::make_shared<playlist_view>(api, p);
}

} // namespace ui
} // namespace spotifar