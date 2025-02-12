#include "events.hpp"
#include "utils.hpp"
#include "config_dialog.hpp"

namespace spotifar { namespace ui {

namespace events {

    void show_root_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_root_view);
    }

    void show_artists_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_artists_view);
    }

    void show_playlists_view()
    {
        return ObserverManager::notify(&ui_events_observer::show_playlists_view);
    }

    void show_playlist_view(const spotify::playlist &playlist)
    {
        return ObserverManager::notify(&ui_events_observer::show_playlist_view, playlist);
    }
    
    void show_artist_view(const spotify::artist &artist)
    {
        return ObserverManager::notify(&ui_events_observer::show_artist_view, artist);
    }
    
    void show_album_view(const spotify::artist &artist, const spotify::album &album)
    {
        return ObserverManager::notify(&ui_events_observer::show_album_view, artist, album);
    }
    
    void show_player_dialog()
    {
        return ObserverManager::notify(&ui_events_observer::show_player_dialog);
    }
    
    void show_config_dialog()
    {
        config_dialog::show();
    }

} // namespace events

} // namespace ui
} // namespace spotiar