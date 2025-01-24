#include "root.hpp"
#include "artists.hpp"
#include "playlists.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;
        
        RootView::RootView(spotify::Api *api):
            View(get_msg(MPanelRootItemLabel)),
            api(api)
        {
        }

        View::Items RootView::get_items()
        {
            Items result =
            {
                {
                    ArtistsView::ID,
                    get_msg(MPanelArtistsItemLabel),
                    get_msg(MPanelArtistsItemDescr),
                    FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
                },
                {
                    PlaylistsView::ID,
                    get_msg(MPanelPlaylistsItemLabel),
                    get_msg(MPanelPlaylistsItemDescr),
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

        std::shared_ptr<RootView> RootView::create_view(spotify::Api *api)
        {
            return std::make_shared<RootView>(api);
        }
    }
}