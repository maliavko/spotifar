#include "artists.hpp"
#include "root.hpp"
#include "artist.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;

        ArtistsView::ArtistsView(spotify::Api *api):
            View(get_msg(MPanelArtistsItemLabel)),
            api(api)
        {
        }

        View::Items ArtistsView::get_items()
        {
            Items result;
            for (const auto &a: api->get_library().get_followed_artist())
                result.push_back({a.id, a.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
            return result;
        }

        std::shared_ptr<View> ArtistsView::select_item(const ItemFarUserData *data)
        {
            if (data == nullptr)
                return RootView::create_view(api);
            // TODO: tmp, cache a mapping in library and use it
            const auto &artists = api->get_library().get_followed_artist();
            const auto artist = std::find_if(artists.begin(), artists.end(), [&data](const auto &a) { return a.id == data->id; });
            return ArtistView::create_view(api, *artist);
        }
        
        std::shared_ptr<ArtistsView> ArtistsView::create_view(spotify::Api *api)
        {
            return std::make_shared<ArtistsView>(api);
        }
    }
}