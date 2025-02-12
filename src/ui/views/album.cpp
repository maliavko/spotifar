#include "album.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

album_view::album_view(spotify::api *api, const spotify::artist &artist, const spotify::album &album,
                        const spotify::track &initial_track):
    view(album.get_user_name()),
    api(api),
    album(album),
    artist(artist),
    initial_track(initial_track)
{
}

view::view_items_t album_view::get_items()
{
    // TODO: tmp code
    view_items_t result;
    for (const auto &track: api->get_album_tracks(album.id))
    {
        wstring track_name = std::format(L"{:02}. {}", track.track_number, track.name);
        result.push_back({
            track.id, track_name, L"",
            FILE_ATTRIBUTE_VIRTUAL,
            (size_t)track.duration,
            track.id == initial_track.id
        });
    }
    return result;
}

intptr_t album_view::select_item(const string &track_id)
{
    if (track_id.empty())
    {
        events::show_artist_view(artist);
        return TRUE;
    }
    else
    {
        api->start_playback(album.id, track_id);
    }
    
    return FALSE;
}

} // namespace ui
} // namespace spotifar