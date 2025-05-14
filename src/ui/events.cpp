#include "events.hpp"
#include "dialogs/menus.hpp"
#include "views/root.hpp"
#include "views/artists.hpp"
#include "views/playlists.hpp"
#include "views/playlist.hpp"
#include "views/albums.hpp"
#include "views/tracks.hpp"

namespace spotifar { namespace ui {

using utils::far3::synchro_tasks::dispatch_event;

namespace events {

    template <class T, typename... MethodArgumentTypes>
    static void show_panel_view(MethodArgumentTypes... args)
    {
        return ObserverManager::notify(
            &ui_events_observer::show_view, std::make_shared<T>(args...));
    }

    template <class T, typename... MethodArgumentTypes>
    static std::shared_ptr<T> view_factory(api_proxy_ptr api, MethodArgumentTypes... args)
    {
        return std::make_shared<T>(api, args...);
    }

    void show_root(api_proxy_ptr api)
    {
        return show_panel_view<root_view>(api);
    }

    void show_collection(api_proxy_ptr api)
    {
        return ObserverManager::notify(
            &ui_events_observer::show_fildered_view,
            ui_events_observer::view_filter_callbacks{
                .artists = &view_factory<followed_artists_view>,
                .albums = &view_factory<saved_albums_view>,
                .tracks = &view_factory<saved_tracks_view>,
                .playlists = &view_factory<saved_playlists_view>,
            });
    }

    void show_browse(api_proxy_ptr api)
    {
        return show_panel_view<browse_view>(api);
    }

    void show_recents(api_proxy_ptr api)
    {
        return ObserverManager::notify(
            &ui_events_observer::show_fildered_view,
            ui_events_observer::view_filter_callbacks{
                .artists = &view_factory<recent_artists_view>,
                .albums = &view_factory<recent_albums_view>,
                .tracks = &view_factory<recent_tracks_view>,
                .playlists = &view_factory<recent_playlists_view>,
            });
    }
    
    void show_new_releases(api_proxy_ptr api)
    {
        return show_panel_view<new_releases_view>(api);
    }
    
    void show_featuring_albums(api_proxy_ptr api)
    {
        return show_panel_view<featuring_albums_view>(api);
    }
    
    void show_featuring_artists(api_proxy_ptr api)
    {
    }

    void show_playlist(api_proxy_ptr api, const playlist_t &playlist)
    {
        return show_panel_view<playlist_view>(api, playlist);
    }
    
    void show_artist_albums(api_proxy_ptr api, const artist_t &artist,
        view_abstract::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        return show_panel_view<artist_view>(api, artist, callback);
    }
    
    void show_album_tracks(api_proxy_ptr api, const album_t &album,
        view_abstract::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        return show_panel_view<album_tracks_view>(api, album, callback);
    }
    
    void show_player()
    {
        return dispatch_event(&ui_events_observer::show_player);
    }
    
    void show_settings()
    {
        show_settings_menu();
    }
    
    void refresh_panels(const string &item_id)
    {
        return dispatch_event(&ui_events_observer::refresh_panels, item_id);
    }

} // namespace events

} // namespace ui
} // namespace spotiar