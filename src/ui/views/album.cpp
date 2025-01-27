#include "album.hpp"
#include "artist.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;
        
        AlbumView::AlbumView(spotify::Api *api, const std::string &album_id,
                             const std::string &artist_id):
            View(get_msg(MPanelAlbumItemLabel)),
            api(api),
            album_id(album_id),
            artist_id(artist_id)
        {
        }

        View::Items AlbumView::get_items()
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, track]: api->get_tracks(album_id))
            {
                std::wstring track_name = std::format(L"{:02}. {}", track.track_number, track.name);
                result.push_back({id, track_name, L"", FILE_ATTRIBUTE_VIRTUAL, (size_t)track.duration});
            }
            return result;
        }

        std::shared_ptr<View> AlbumView::select_item(const ItemFarUserData *data)
        {
            if (data == nullptr)
            {
                // TODO: tmp, cache a mapping in library and use it
                const auto &artists = api->get_library().get_followed_artist();
                const auto artist = std::find_if(artists.begin(), artists.end(), [this](const auto &a) { return a.id == artist_id; });
                return ArtistView::create_view(api, *artist);
            }

            api->start_playback(album_id, data->id);
            
            return nullptr;
        }
        
        std::shared_ptr<AlbumView> AlbumView::create_view(spotify::Api *api,
            const std::string &album_id, const std::string &artist_id)
        {
            return std::make_shared<AlbumView>(api, album_id, artist_id);
        }
    }
}