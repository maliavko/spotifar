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

    void show_featuring(api_proxy_ptr api);

    void show_playlist(api_proxy_ptr api, const playlist_t &playlist);

    void show_artist_albums(api_proxy_ptr api, const artist_t &artist, view_abstract::return_callback_t callback = {});

    void show_album_tracks(api_proxy_ptr api, const album_t &album, view_abstract::return_callback_t callback = {});
    
    void show_player();
    
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

        const static size_t artists_idx = 0;
        const static size_t albums_idx = 1;
        const static size_t tracks_idx = 2;
        const static size_t playlists_idx = 3;

        callback_t artists = nullptr;
        callback_t albums = nullptr;
        callback_t tracks = nullptr;
        callback_t playlists = nullptr;

        void clear()
        {
            artists = albums = tracks = playlists = nullptr;
        }

        callback_t get_callback(size_t idx)
        {
            switch (idx)
            {
                case artists_idx: return artists;
                case albums_idx: return albums;
                case tracks_idx: return tracks;
                case playlists_idx: return playlists;
            }
            return artists;
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