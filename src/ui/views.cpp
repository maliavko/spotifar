#include "ui/views.hpp"
#include "lng.hpp"
#include "config.hpp"

namespace spotifar
{
    namespace ui
    {
        using config::get_msg;

        std::shared_ptr<RootView> create_root_view()
        {
            return std::make_shared<RootView>();
        }

        std::shared_ptr<ArtistsView> create_artists_view()
        {
            return std::make_shared<ArtistsView>();
        }

        std::shared_ptr<PlaylistsView> create_playlists_view()
        {
            return std::make_shared<PlaylistsView>();
        }

        std::shared_ptr<ArtistView> create_artist_view(const std::string& artist_id)
        {
            return std::make_shared<ArtistView>(artist_id);
        }

        std::shared_ptr<AlbumView> create_album_view(const std::string& album_id, const std::string& artist_id)
        {
            return std::make_shared<AlbumView>(album_id, artist_id);
        }

        View::View(wstring name):
            name(name)
        {

        }

        View::~View()
        {
            
        }

        RootView::RootView():
            View(get_msg(MPanelRootItemLabel))
        {
        }

        View::Items RootView::get_items(Api& api) const
        {
            Items result =
            {
                {
                    ARTISTS_VIEW_ID,
                    get_msg(MPanelArtistsItemLabel),
                    get_msg(MPanelArtistsItemDescr)
                },
                {
                    PLAYLIST_VIEW_ID,
                    get_msg(MPanelPlaylistsItemLabel),
                    get_msg(MPanelPlaylistsItemDescr),
                }
            };

            return result;
        }

        std::shared_ptr<View> RootView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data->id == ARTISTS_VIEW_ID)
                return create_artists_view();
            
            if (data->id == PLAYLIST_VIEW_ID)
                return create_playlists_view();

            return NULL;
        }
        
        ArtistsView::ArtistsView():
            View(get_msg(MPanelArtistsItemLabel))
        {
        }

        View::Items ArtistsView::get_items(Api& api) const
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, a]: api.get_artist())
            {
                result.push_back({
                    id, utils::to_wstring(a.name), L""
                });
            }
            return result;
        }

        std::shared_ptr<View> ArtistsView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return create_root_view();
            return create_artist_view(data->id);
        }
        
        ArtistView::ArtistView(const std::string& artist_id):
            View(get_msg(MPanelArtistItemLabel)),
            artist_id(artist_id)
        {
        }

        View::Items ArtistView::get_items(Api& api) const
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, a]: api.get_albums(artist_id))
            {
                result.push_back({
                    id, utils::to_wstring(a.name), L""
                });
            }
            return result;	
        }

        std::shared_ptr<View> ArtistView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return create_artists_view();
            return create_album_view(data->id, artist_id);
        }
        
        AlbumView::AlbumView(const std::string& album_id, const std::string& artist_id):
            View(get_msg(MPanelAlbumItemLabel)),
            album_id(album_id),
            artist_id(artist_id)
        {
        }

        View::Items AlbumView::get_items(Api& api) const
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, a]: api.get_tracks(album_id))
            {
                std::string track_user_name = std::format("{:2}. {}", a.track_number, a.name);
                result.push_back({
                    id, utils::to_wstring(track_user_name), L""
                });
            }
            return result;	
        }

        std::shared_ptr<View> AlbumView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return create_artist_view(artist_id);

            // TODO: for some reason does not work, worked before
            //api.start_playback(album_id, data->id);
            
            return NULL;
        }
        
        PlaylistsView::PlaylistsView():
            View(get_msg(MPanelPlaylistsItemLabel))
        {
        }

        View::Items PlaylistsView::get_items(Api& api) const
        {
            // TODO: tmp code
            Items result =
            {
                {
                    "playlist1",
                    L"Playlist1",
                    L"Playlist1"
                },
                {
                    "playlist2",
                    L"Playlist2",
                    L"Playlist2"
                }
            };
            return result;	
        }

        std::shared_ptr<View> PlaylistsView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return create_root_view();
            return NULL;
        }
    }
}