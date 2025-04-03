#ifndef EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#define EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#pragma once

#include "stdafx.h"
#include "spotify/items.hpp"
#include "spotify/abstract.hpp"
#include "ui/views/view.hpp"

namespace spotifar { namespace ui {

using spotify::api_proxy_ptr;

/// @brief Global ui commands to show views or persome some operations on them,
/// accessible without instances
namespace events {

    void show_root(api_proxy_ptr api);

    // collection menu events
    void show_collections(api_proxy_ptr api);
    void show_followed_artists(api_proxy_ptr api);
    void show_saved_albums(api_proxy_ptr api);
    void show_saved_tracks(api_proxy_ptr api);
    void show_saved_playlists(api_proxy_ptr api);

    // recent menu events
    void show_recents(api_proxy_ptr api);
    void show_recent_tracks(api_proxy_ptr api);
    void show_recent_albums(api_proxy_ptr api);
    void show_recent_artists(api_proxy_ptr api);
    void show_recent_playlists(api_proxy_ptr api);

    // browse menu events
    void show_browse(api_proxy_ptr api);
    void show_new_releases(api_proxy_ptr api);
    void show_featuring_albums(api_proxy_ptr api);
    void show_featuring_artists(api_proxy_ptr api);

    void show_playlist(api_proxy_ptr api, const playlist_t &playlist);

    void show_artist_albums(api_proxy_ptr api, const artist_t &artist, view_abstract::return_callback_t callback = {});

    void show_album_tracks(api_proxy_ptr api, const album_t &album, view_abstract::return_callback_t callback = {});
    
    void show_player();
    
    void show_config();

    void refresh_panels(const string &item_id = "");

} // namespace events

/// @brief A panel class implements this interface to listen global
/// commands from `events` namespace and show appropriate requested
/// views.
struct ui_events_observer: public BaseObserverProtocol
{
    /// @brief Show playback player
    virtual void show_player() {}

    /// @brief Refreshes the plugin's panel, forcing it to repopulate items
    /// and redraw it. If `item_id` is given, the one will be selected
    /// on the panel after operation is finished
    virtual void refresh_panels(const item_id_t &item_id) {}

    /// @brief Sends a `view` instance to the panel to show it
    virtual void show_panel_view(view_ptr view) {}

    virtual void on_show_filters_menu() {};
};

} // namespace ui
} // namespace spotiar

#endif // EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3