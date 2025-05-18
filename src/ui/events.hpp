#ifndef EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#define EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#pragma once

#include "stdafx.h"
#include "spotify/items.hpp"
#include "spotify/common.hpp"
#include "ui/views/view.hpp"

namespace spotifar { namespace ui {

using spotify::api_proxy_ptr;

/// @brief Global ui commands to show views or persome some operations on them,
/// accessible without instances
namespace events {

    void show_root(api_proxy_ptr api);

    void show_collection(api_proxy_ptr api);

    void show_recents(api_proxy_ptr api);

    void show_browse(api_proxy_ptr api);

    void show_new_releases(api_proxy_ptr api);

    void show_recently_liked_tracks(api_proxy_ptr api);

    void show_recently_saved_albums(api_proxy_ptr api);

    void show_playlist(api_proxy_ptr api, const playlist_t &playlist);

    void show_artist_albums(api_proxy_ptr api, const artist_t &artist, view_abstract::return_callback_t callback = {});

    void show_album_tracks(api_proxy_ptr api, const album_t &album, view_abstract::return_callback_t callback = {});
    
    void show_player();

    void show_playing_queue(api_proxy_ptr api);
    
    void show_settings();

    void refresh_panels(const string &item_id = "");

} // namespace events

/// @brief A panel class implements this interface to listen global
/// commands from `events` namespace and show appropriate requested
/// views.
struct ui_events_observer: public BaseObserverProtocol
{
    struct view_filter_callbacks
    {
        using callback_t = std::function<view_ptr(api_proxy_ptr)>;

        const static size_t
            artists_idx = 0,
            albums_idx = 1,
            tracks_idx = 2,
            playlists_idx = 3;

        callback_t artists, albums, tracks, playlists;
        size_t default_view_idx = artists_idx;

        void clear()
        {
            artists = albums = tracks = playlists = nullptr;
        }

        callback_t get_callback(size_t view_idx)
        {
            std::vector<callback_t> callbacks{ artists, albums, tracks, playlists };

            // checking the callback of a given `view_idx` first;
            // if it is invalid, checking then the default view callback
            for (auto idx: { view_idx, default_view_idx })
            {
                if (idx < callbacks.size() && callbacks[idx])
                    return callbacks[idx];
            }

            // ...if none of the indecies worked, trying to return the first valid one
            for (size_t idx = 0; idx < callbacks.size(); idx++)
                if (callbacks[idx])
                    return callbacks[idx];

            return nullptr;
        }
    };

    /// @brief Show playback player
    virtual void show_player() {}

    /// @brief Refreshes the plugin's panel, forcing it to repopulate items
    /// and redraw it. If `item_id` is given, the one will be selected
    /// on the panel after operation is finished
    virtual void refresh_panels(const item_id_t &item_id) {}

    /// @brief Sends a `view` instance to the panel to show it
    virtual void show_view(view_ptr view) {}

    virtual void show_fildered_view(view_filter_callbacks callbacks) {}
    
    virtual void on_show_filters_menu() {}
};

} // namespace ui
} // namespace spotiar

#endif // EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3