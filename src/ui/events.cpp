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

    static void show_view(ui_events_observer::view_builder_t &&builder)
    {
        ObserverManager::notify(&ui_events_observer::show_view, PANEL_ACTIVE, builder);
    }

    static void show_multiview(ui_events_observer::multiview_builder_t &&callbacks)
    {
        ObserverManager::notify(&ui_events_observer::show_multiview, PANEL_ACTIVE, callbacks);
    }

    void show_root(api_weak_ptr_t api)
    {
        show_view(get_builder<root_view>(api));
    }

    void show_collection(api_weak_ptr_t api)
    {
        show_multiview({
            .artists = get_builder<followed_artists_view>(api),
            .albums = get_builder<saved_albums_view>(api),
            .tracks = get_builder<saved_tracks_view>(api),
            .playlists = get_builder<saved_playlists_view>(api),
            .default_view_idx = ui_events_observer::multiview_builder_t::artists_idx
        });
    }

    void show_browse(api_weak_ptr_t api)
    {
        show_view(get_builder<browse_view>(api));
    }

    void show_recents(api_weak_ptr_t api)
    {
        show_multiview({
            .artists = get_builder<recent_artists_view>(api),
            .albums = get_builder<recent_albums_view>(api),
            .tracks = get_builder<recent_tracks_view>(api),
            .playlists = get_builder<recent_playlists_view>(api),
            .default_view_idx = ui_events_observer::multiview_builder_t::tracks_idx
        });
    }
    
    void show_new_releases(api_weak_ptr_t api)
    {
        show_view(get_builder<new_releases_view>(api));
    }
    
    void show_recently_liked_tracks(api_weak_ptr_t api)
    {
        show_multiview({
            .artists = get_builder<recently_liked_tracks_artists_view>(api),
            .albums = get_builder<recently_liked_tracks_albums_view>(api),
            .tracks = get_builder<recently_liked_tracks_view>(api),
            .default_view_idx = ui_events_observer::multiview_builder_t::tracks_idx
        });
    }
    
    void show_recently_saved_albums(api_weak_ptr_t api)
    {
        show_multiview({
            .artists = get_builder<recently_saved_album_artists_view>(api),
            .albums = get_builder<recently_saved_albums_view>(api),
            .default_view_idx = ui_events_observer::multiview_builder_t::albums_idx
        });
    }

    void show_playlist(api_weak_ptr_t api, const playlist_t &playlist)
    {
        show_view(get_builder<playlist_view>(api, playlist));
    }
    
    void show_artist_albums(api_weak_ptr_t api, const artist_t &artist,
        view_abstract::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        show_view(get_builder<artist_view>(api, artist, callback));
    }
    
    void show_album_tracks(api_weak_ptr_t api, const album_t &album,
        view_abstract::return_callback_t callback)
    {
        if (!callback)
            callback = std::bind(show_root, api);
        
        show_view(get_builder<album_tracks_view>(api, album, callback));
    }
    
    void show_player()
    {
        dispatch_event(&ui_events_observer::show_player);
    }
    
    void show_playing_queue(api_weak_ptr_t api)
    {
        show_view(get_builder<playing_queue_view>(api));
    }
    
    void show_settings()
    {
        show_settings_menu();
    }

    void select_item(const string &item_id)
    {
        dispatch_event(&ui_events_observer::refresh_panels, PANEL_ACTIVE, item_id);
    }

    void refresh_panel(HANDLE panel)
    {
        dispatch_event(&ui_events_observer::refresh_panels, panel, "");
    }
    
    void refresh_panels()
    {
        for (const auto &panel: { PANEL_ACTIVE, PANEL_PASSIVE })
            utils::far3::panels::redraw(panel);
    }
    
    void quit()
    {
        dispatch_event(&ui_events_observer::close_panel, (HANDLE)NULL);
    }

    void close_panel(HANDLE panel)
    {
        dispatch_event(&ui_events_observer::close_panel, panel);
    }
    
    void show_filters_menu()
    {
        dispatch_event(&ui_events_observer::show_filters_menu);
    }

} // namespace events
} // namespace ui
} // namespace spotiar