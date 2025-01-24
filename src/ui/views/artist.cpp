#include "artist.hpp"
#include "artists.hpp"
#include "album.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;
        
        ArtistView::ArtistView(spotify::Api *api, const std::string& artist_id):
            View(get_msg(MPanelArtistItemLabel)),
            api(api),
            artist_id(artist_id)
        {
        }

        View::Items ArtistView::get_items()
        {
            // TODO: split albums and singles into separate directoriess
            Items result;
            for (auto& [id, a]: api->get_albums(artist_id))
            {
                std::wstring album_name = std::format(L"[{}] {}", utils::to_wstring(a.get_release_year()), a.name);
                if (a.is_single())
                    album_name += L" [EP]";
                
                result.push_back({id, album_name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
            }
            return result;	
        }

        std::shared_ptr<View> ArtistView::select_item(const ItemFarUserData *data)
        {
            if (data == nullptr)
                return ArtistsView::create_view(api);
            return AlbumView::create_view(api, data->id, artist_id);
        }

        std::shared_ptr<ArtistView> ArtistView::create_view(
            spotify::Api *api, const std::string &artist_id)
        {
            return std::make_shared<ArtistView>(api, artist_id);
        }
    }
}