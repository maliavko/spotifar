#include "album.hpp"
#include "artist.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

album_view::album_view(spotify::api *api, const spotify::album &album,
                       const spotify::artist &artist):
    view(album.get_user_name()),
    api(api),
    album(album),
    artist(artist)
{
}

view::view_items_t album_view::get_items()
{
    view_items_t result;
    for (const auto &track: api->get_library().get_album_tracks(album.id))
    {
        wstring track_name = std::format(L"{:02}. {}", track.track_number, track.name);
        result.push_back({track.id, track_name, L"", FILE_ATTRIBUTE_VIRTUAL, (size_t)track.duration});
    }
    return result;
}

std::shared_ptr<view> album_view::select_item(const string &track_id)
{
    if (track_id.empty())
    {
        return artist_view::build(api, artist);
    }
    else
    {
        api->start_playback(album.id, track_id);
    }
    
    return nullptr;
}

std::shared_ptr<album_view> album_view::build(spotify::api *api,
    const spotify::album &album, const spotify::artist &artist)
{
    return std::make_shared<album_view>(api, album, artist);
}

} // namespace ui
} // namespace spotifar