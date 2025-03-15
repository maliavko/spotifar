#include "events.hpp"
#include "dialogs/menus.hpp"
#include "views/root.hpp"
#include "views/artists.hpp"
#include "views/playlists.hpp"
#include "views/playlist.hpp"
#include "views/artist.hpp"
#include "views/album.hpp"

namespace spotifar { namespace ui {

namespace events {

    template <class T, typename... MethodArgumentTypes>
    static void show_panel_view(MethodArgumentTypes... args)
    {
        return ObserverManager::notify(
            &ui_events_observer::show_panel_view, std::make_shared<T>(args...));
    }

    void show_collection_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_collection_view);
    }

    void show_albums_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_albums_view);
    }

    void show_tracks_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_tracks_view);
    }

    void show_browse_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_browse_view);
    }

    void show_recents_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_playlist_view, playlist);
    }
    
    void show_artist_view(api_abstract *api, const artist &artist)
    {
        return ObserverManager::notify(&ui_events_observer::show_albums_view);
    }

    void show_tracks_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_tracks_view);
    }

    void show_playlists_view(api_abstract *api)
    {
        return show_panel_view<playlists_view>(api);
    }

    void show_playlist_view(api_abstract *api, const playlist &playlist)
    {
        return show_panel_view<playlist_view>(api, playlist);
    }
    
    void show_artist_view(api_abstract *api, const artist &artist)
    {
        return show_panel_view<artist_view>(api, artist);
    }
    
    void show_album_view(api_abstract *api, const album &album)
    {
        return show_panel_view<album_view>(api, album);
    }
    
    void show_recents_view(api_abstract *api)
    {
        //TODO: unfinished
        //return show_panel_view<album_view>(api, album);
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