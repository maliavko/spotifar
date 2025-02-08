#include "playlists.hpp"
#include "root.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_text;

        PlaylistsView::PlaylistsView(spotify::api *api):
            View(get_text(MPanelPlaylistsItemLabel)),
            api(api)
        {
        }

        View::Items PlaylistsView::get_items()
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, a]: api->get_playlists())
                result.push_back({id, a.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
            return result;
        }

        std::shared_ptr<View> PlaylistsView::select_item(const ItemFarUserData* data)
        {
            if (data == nullptr)
                return RootView::create_view(api);
            return NULL;
        }

        std::shared_ptr<PlaylistsView> PlaylistsView::create_view(spotify::api *api)
        {
            return std::make_shared<PlaylistsView>(api);
        }
    }
}