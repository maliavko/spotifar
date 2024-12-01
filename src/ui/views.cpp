#include "ui/views.hpp"
#include "lng.hpp"
#include "config.hpp"

namespace spotifar
{
    namespace ui
    {
        using config::get_msg;

        View::View(wstring name):
            name(name)
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

        std::string RootView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data->id == ARTISTS_VIEW_ID && data->id == PLAYLIST_VIEW_ID)
            {
                return data->id;
            }
            return NONE_VIEW_ID;
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

        std::string ArtistsView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return ROOT_VIEW_ID;
            return ARTIST_VIEW_ID;
        }
        
        ArtistView::ArtistView(const std::string& artist_id):
            View(get_msg(MPanelArtistsItemLabel)),
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

        std::string ArtistView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return ARTISTS_VIEW_ID;

            browser.gotoAlbum(data->id, artist_id);
            return ALBUM_VIEW_ID;
        }
        
        AlbumView::AlbumView(const std::string& id, const std::string& artist_id):
            View(get_msg(MPanelArtistsItemLabel)),
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

        std::string AlbumView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
            {
                browser.gotoArtist(artist_id);
                return true;
            }

            api.start_playback(album_id, data->id);
            
            return false;
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

        std::string PlaylistsView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
            {
                browser.gotoRootMenu();
                return true;
            }
            return false;
        }
    }
}