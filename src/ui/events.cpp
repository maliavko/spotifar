#include "events.hpp"
#include "dialogs/menus.hpp"
#include "views/root.hpp"
#include "views/artists.hpp"
#include "views/playlists.hpp"
#include "views/albums.hpp"
#include "views/tracks.hpp"

namespace spotifar { namespace ui { namespace events {
    
using utils::far3::synchro_tasks::dispatch_event;

template <class ViewT, typename... ArgsT>
static auto get_builder(ArgsT... args) -> view_builder_t
{
    return [...args = std::forward<ArgsT>(args)]
        (HANDLE panel) {
            return std::make_shared<ViewT>(panel, args...);
        };
}

static void show_view(view_builder_t &&builder, view::return_callback_t callback)
{
    ObserverManager::notify(&ui_events_observer::show_view, PANEL_ACTIVE, builder, callback);
}

static void show_multiview(multiview_builder_t &&builders, view::return_callback_t callback)
{
    ObserverManager::notify(&ui_events_observer::show_multiview, PANEL_ACTIVE, builders, callback);
}

void show_root(api_weak_ptr_t api)
{
    show_view(get_builder<root_view>(api), {});
}

void show_collection(api_weak_ptr_t api, int page_idx)
{
    auto settings = config::get_multiview_settings("collection", { multiview_builder_t::artists_idx });

    if (page_idx > -1)
        settings->idx = page_idx;
    
    show_multiview(
        {
            .artists = get_builder<followed_artists_view>(api),
            .albums = get_builder<saved_albums_view>(api),
            .tracks = get_builder<saved_tracks_view>(api),
            .playlists = get_builder<saved_playlists_view>(api),
            .settings = settings
        },
        [api] { show_root(api); }
    );
}

void show_browse(api_weak_ptr_t api)
{
    show_view(get_builder<browse_view>(api), [api] { show_root(api); });
}

void show_recents(api_weak_ptr_t api)
{
    show_multiview(
        {
            .artists = get_builder<recent_artists_view>(api),
            .albums = get_builder<recent_albums_view>(api),
            .tracks = get_builder<recent_tracks_view>(api),
            .playlists = get_builder<recent_playlists_view>(api),
            .settings = config::get_multiview_settings(
                "recents", { multiview_builder_t::tracks_idx })
        },
        [api] { show_root(api); }
    );
}

void show_new_releases(api_weak_ptr_t api)
{
    show_view(get_builder<new_releases_view>(api), [api] { show_browse(api); });
}

void show_recently_saved(api_weak_ptr_t api)
{
    show_multiview(
        {
            .albums = get_builder<recently_saved_albums_view>(api),
            .tracks = get_builder<recently_liked_tracks_view>(api),
            .settings = config::get_multiview_settings(
                "recently_saved", { multiview_builder_t::tracks_idx })
        },
        [api] { show_browse(api); }
    );
}

void show_user_top_items(api_weak_ptr_t api)
{
    show_multiview(
        {
            .artists = get_builder<user_top_artists_view>(api),
            .tracks = get_builder<user_top_tracks_view>(api),
            .settings = config::get_multiview_settings(
                "user_top", { multiview_builder_t::tracks_idx })
        },
        [api] { show_browse(api); }
    );
}

void show_playlist(api_weak_ptr_t api, const playlist_t &playlist)
{
    show_view(get_builder<playlist_view>(api, playlist), [api] { show_collection(api); });
}

void show_artist(api_weak_ptr_t api, const artist_t &artist, view::return_callback_t callback)
{
    if (!callback)
        callback = std::bind(show_root, api);
    
    show_multiview(
        {
            .albums = get_builder<artist_albums_view>(api, artist),
            .tracks = get_builder<artist_top_tracks_view>(api, artist),
            .settings = config::get_multiview_settings("artist", { multiview_builder_t::albums_idx })
        },
        callback
    );
}

void show_album_tracks(api_weak_ptr_t api, const simplified_album_t &album, view::return_callback_t callback)
{
    if (!callback)
        callback = std::bind(show_root, api);
    
    show_view(get_builder<album_tracks_view>(api, album), callback);
}

void show_player()
{
    dispatch_event(&ui_events_observer::show_player);
}

void show_playing_queue(api_weak_ptr_t api)
{
    show_view(get_builder<playing_queue_view>(api), [api] { show_root(api); });
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