#include "events.hpp"
#include "dialogs/menus.hpp"
#include "views/root.hpp"
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

    void show_root_view(api_abstract *api)
    {
        return show_panel_view<root_view>(api);
    }

    void show_collections_view(api_abstract *api)
    {
        return show_panel_view<collection_view>(api);
    }

    void show_artists_collection_view(api_abstract *api)
    {
        return show_panel_view<artists_view>(api);
    }

    void show_albums_collection_view(api_abstract *api)
    {
        return show_panel_view<albums_collection_view>(api);
    }

    void show_tracks_collection_view(api_abstract *api)
    {
        // TODO: unfinished
    }

    void show_browse_view(api_abstract *api)
    {
        // TODO: unfinished
    }

    void show_recents_view(api_abstract *api)
    {
        // TODO: unfinished
    }

    void show_playlists_view(api_abstract *api)
    {
        return show_panel_view<playlists_view>(api);
    }

    void show_playlist_view(api_abstract *api, const playlist_t &playlist)
    {
        return show_panel_view<playlist_view>(api, playlist);
    }
    
    void show_artist_view(api_abstract *api, const artist_t &artist)
    {
        return show_panel_view<artist_view>(api, artist);
    }
    
    void show_album_tracks_view(api_abstract *api, const album_t &album)
    {
        return show_panel_view<album_tracks_view>(api, album);
    }
    
    void show_player_dialog()
    {
        return ObserverManager::notify(&ui_events_observer::show_player_dialog);
    }
    
    void show_config_dialog()
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