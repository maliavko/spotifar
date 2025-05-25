#include "events.hpp"
#include "dialogs/menus.hpp"
#include "views/root.hpp"
#include "views/artists.hpp"
#include "views/playlists.hpp"
#include "views/playlist.hpp"
#include "views/albums.hpp"
#include "views/tracks.hpp"

namespace spotifar { namespace ui { namespace events {
    
    using utils::far3::synchro_tasks::dispatch_event;

    template <class ViewT, typename... ArgsT>
    static auto get_builder(ArgsT... args) -> ui_events_observer::view_builder_t
    {
        return [...args = std::forward<ArgsT>(args)]
            (HANDLE panel) {
                return std::make_shared<ViewT>(panel, args...);
            };
    }

    static void switch_view(ui_events_observer::view_builder_t &&builder)
    {
        ObserverManager::notify(&ui_events_observer::switch_view, builder);
    }

    static void switch_filtered_view(ui_events_observer::view_filter_callbacks &&callbacks)
    {
        ObserverManager::notify(&ui_events_observer::switch_filtered_view, callbacks);
    }

    void show_root(api_proxy_ptr api)
    {
        switch_view(get_builder<root_view>(api));
    }

    void show_collection(api_proxy_ptr api)
    {
        switch_filtered_view({
            .artists = get_builder<followed_artists_view>(api),
            .albums = get_builder<saved_albums_view>(api),
            .tracks = get_builder<saved_tracks_view>(api),
            .playlists = get_builder<saved_playlists_view>(api),
            .default_view_idx = ui_events_observer::view_filter_callbacks::artists_idx
        });
    }

    void show_browse(api_proxy_ptr api)
    {
        switch_view(get_builder<browse_view>(api));
    }

    void show_recents(api_proxy_ptr api)
    {
        switch_filtered_view({
            .artists = get_builder<recent_artists_view>(api),
            .albums = get_builder<recent_albums_view>(api),
            .tracks = get_builder<recent_tracks_view>(api),
            .playlists = get_builder<recent_playlists_view>(api),
            .default_view_idx = ui_events_observer::view_filter_callbacks::tracks_idx
        });
    }
    
    void show_new_releases(api_proxy_ptr api)
    {
        switch_view(get_builder<new_releases_view>(api));
    }
    
    void show_recently_liked_tracks(api_proxy_ptr api)
    {
        switch_filtered_view({
            .artists = get_builder<recently_liked_tracks_artists_view>(api),
            .albums = get_builder<recently_liked_tracks_albums_view>(api),
            .tracks = get_builder<recently_liked_tracks_view>(api),
            .default_view_idx = ui_events_observer::view_filter_callbacks::tracks_idx
        });
    }
    
    void show_recently_saved_albums(api_proxy_ptr api)
    {
        switch_filtered_view({
            .artists = get_builder<recently_saved_album_artists_view>(api),
            .albums = get_builder<recently_saved_albums_view>(api),
            .default_view_idx = ui_events_observer::view_filter_callbacks::albums_idx
        });
    }

    void show_playlist(api_proxy_ptr api, const playlist_t &playlist)
    {
        switch_view(get_builder<playlist_view>(api, playlist));
    }
    
    void show_artist_albums(api_proxy_ptr api, const artist_t &artist,
        view_abstract::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        switch_view(get_builder<artist_view>(api, artist, callback));
    }
    
    void show_album_tracks(api_proxy_ptr api, const album_t &album,
        view_abstract::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        switch_view(get_builder<album_tracks_view>(api, album, callback));
    }
    
    void show_player()
    {
        dispatch_event(&ui_events_observer::show_player);
    }
    
    void show_playing_queue(api_proxy_ptr api)
    {
        switch_view(get_builder<playing_queue_view>(api));
    }
    
    void show_settings()
    {
        show_settings_menu();
    }
    
    void refresh_panels(const string &item_id)
    {
        dispatch_event(&ui_events_observer::refresh_panels, item_id);
    }
    
    void quit()
    {
        dispatch_event(&ui_events_observer::quit);
    }

} // namespace events
} // namespace ui
} // namespace spotiar