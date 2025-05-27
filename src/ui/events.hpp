#ifndef EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#define EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#pragma once

#include "stdafx.h"
#include "spotify/items.hpp"
#include "spotify/common.hpp"
#include "ui/views/view.hpp"

namespace spotifar { namespace ui {

using spotify::api_weak_ptr_t;

/// @brief Global ui commands to show views or persome some operations on them,
/// accessible without instances
namespace events {

    void show_root(api_weak_ptr_t api);

    void show_collection(api_weak_ptr_t api);

    void show_recents(api_weak_ptr_t api);

    void show_browse(api_weak_ptr_t api);

    void show_new_releases(api_weak_ptr_t api);

    void show_recently_liked_tracks(api_weak_ptr_t api);

    void show_recently_saved_albums(api_weak_ptr_t api);

    void show_playlist(api_weak_ptr_t api, const spotify::playlist_t &playlist);

    void show_artist_albums(api_weak_ptr_t api, const spotify::artist_t &artist,
        view_abstract::return_callback_t callback = {});

    void show_album_tracks(api_weak_ptr_t api, const spotify::album_t &album,
        view_abstract::return_callback_t callback = {});

    void show_playing_queue(api_weak_ptr_t api);
    
    void show_player();
    
    void show_settings();

    /// @brief Forcing the active panel to redraw and to try to find and
    /// set the given `item_id` under cursur
    void select_item(const string &item_id);

    /// @brief Forcing the given `panel` to redraw
    /// @param panel could be a panel handle, PANEL_ACTIVE or PANEL_PASSIVE
    void refresh_panel(HANDLE panel);

    /// @brief Forcing all the plugin's opened panels to redraw
    void refresh_panels();

    /// @brief Forcing all the plugin's opened panels to close and quit plugin
    void quit();

    /// @brief Forcing the given `panel` to close
    /// @param panel could be a panel handle, PANEL_ACTIVE or PANEL_PASSIVE
    void close_panel(HANDLE panel);

    /// @brief Sending an event to the active panel to show it's current view
    /// filters menu dialog
    void show_filters_menu();

} // namespace events

/// @brief A panel class implements this interface to listen global
/// commands from `events` namespace and to show appropriate requested views
struct ui_events_observer: public BaseObserverProtocol
{
    using view_builder_t = std::function<view_ptr_t(HANDLE)>;

    struct multiview_builder_t
    {
        const static size_t
            artists_idx = 0,
            albums_idx = 1,
            tracks_idx = 2,
            playlists_idx = 3;

        view_builder_t artists, albums, tracks, playlists;
        size_t default_view_idx = artists_idx;

        void clear()
        {
            artists = albums = tracks = playlists = nullptr;
        }

        view_builder_t get_builder(size_t view_idx)
        {
            std::vector<view_builder_t> builders{ artists, albums, tracks, playlists };

            // checking the builder of a given `view_idx` first;
            // if it is invalid, checking then the default view builder
            for (auto idx: { view_idx, default_view_idx })
            {
                if (idx < builders.size() && builders[idx])
                    return builders[idx];
            }

            // ...if none of the indicies worked, trying to return the first valid one
            for (size_t idx = 0; idx < builders.size(); idx++)
                if (builders[idx])
                    return builders[idx];

            return nullptr;
        }
    };

    /// @brief Show playback player
    virtual void show_player() {}

    /// @brief Refreshes the plugin's panel, forcing it to repopulate items
    /// and redraw it. If `item_id` is given, the one will be selected
    /// on the panel after operation is finished
    virtual void refresh_panels(HANDLE panel, const spotify::item_id_t &item_id) {}

    virtual void show_view(HANDLE panel, view_builder_t builder) {}

    virtual void show_multiview(HANDLE panel, multiview_builder_t callbacks) {}

    virtual void close_panel(HANDLE panel) {}

    virtual void show_filters_menu() {}
};

} // namespace ui
} // namespace spotiar

#endif // EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3