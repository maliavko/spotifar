#include "events.hpp"
#include "dialogs/menus.hpp"
#include "views/root.hpp"
#include "views/recents.hpp"
#include "views/collection.hpp"
#include "views/artists.hpp"
#include "views/playlists.hpp"
#include "views/playlist.hpp"
#include "views/albums.hpp"
#include "views/tracks.hpp"

namespace spotifar { namespace ui {

namespace events {

    template <class T, typename... MethodArgumentTypes>
    static void show_panel_view(MethodArgumentTypes... args)
    {
        return ObserverManager::notify(
            &ui_events_observer::show_panel_view, std::make_shared<T>(args...));
    }

    void show_root(api_abstract *api)
    {
        return show_panel_view<root_view>(api);
    }

    void show_collections(api_abstract *api)
    {
        return show_panel_view<collection_view>(api);
    }

    void show_artists_collection(api_abstract *api)
    {
        return show_panel_view<followed_artists_view>(api);
    }

    void show_albums_collection(api_abstract *api)
    {
        return show_panel_view<albums_collection_view>(api);
    }

    void show_tracks_collection(api_abstract *api)
    {
        return show_panel_view<saved_tracks_view>(api);
    }

    void show_browse(api_abstract *api)
    {
        // TODO: unfinished
    }

    void show_recents(api_abstract *api)
    {
        return show_panel_view<recents_view>(api);
    }

    void show_recent_tracks(api_abstract *api)
    {
        return show_panel_view<recent_tracks_view>(api);
    }

    void show_recent_albums(api_abstract *api)
    {
        return show_panel_view<recent_albums_view>(api);
    }

    void show_recent_artists(api_abstract *api)
    {
        return show_panel_view<recent_artists_view>(api);
    }

    void show_recent_playlists(api_abstract *api)
    {
        return show_panel_view<recent_playlists_view>(api);
    }
    
    void show_new_releases(api_abstract *api)
    {
        return show_panel_view<new_releases_view>(api);
    }

    void show_playlists_collection(api_abstract *api)
    {
        return show_panel_view<playlists_view>(api);
    }

    void show_playlist(api_abstract *api, const playlist_t &playlist)
    {
        return show_panel_view<playlist_view>(api, playlist);
    }
    
    void show_artist_albums(api_abstract *api, const artist_t &artist,
        view::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        return show_panel_view<artist_view>(api, artist, callback);
    }
    
    void show_album_tracks(api_abstract *api, const album_t &album,
        view::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        return show_panel_view<album_tracks_view>(api, album, callback);
    }
    
    void show_player()
    {
        return ObserverManager::notify(&ui_events_observer::show_player);
    }
    
    void show_config()
    {
        show_config_menu();
    }
    
    void refresh_panels(const string &item_id)
    {
        return ObserverManager::notify(&ui_events_observer::refresh_panels, item_id);
    }

} // namespace events

} // namespace ui
} // namespace spotiar