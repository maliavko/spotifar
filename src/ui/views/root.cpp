#include "root.hpp"
#include "artists.hpp"
#include "playlists.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_text;
        
        RootView::RootView(spotify::api *api):
            View(get_text(MPanelRootItemLabel)),
            api(api)
        {
        }

        View::Items RootView::get_items()
        {
            Items result =
            {
                {
                    ArtistsView::ID,
                    get_text(MPanelArtistsItemLabel),
                    get_text(MPanelArtistsItemDescr),
                    FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
                },
                {
                    PlaylistsView::ID,
                    get_text(MPanelPlaylistsItemLabel),
                    get_text(MPanelPlaylistsItemDescr),
                    FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
                }
            };

            return result;
        }

        std::shared_ptr<View> RootView::select_item(const ItemFarUserData *data)
        {
            if (data->id == ArtistsView::ID)
                return ArtistsView::create_view(api);
            
            if (data->id == PlaylistsView::ID)
                return PlaylistsView::create_view(api);

            return NULL;
        }

        std::shared_ptr<RootView> RootView::create_view(spotify::api *api)
        {
            return std::make_shared<RootView>(api);
        }
    }
}