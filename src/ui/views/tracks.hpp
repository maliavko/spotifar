#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"
#include "spotify/playback.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class tracks_base_view: public view
{
public:
    tracks_base_view(api_abstract *api, const string &view_uid,
        return_callback_t callback);

    auto get_items() -> const items_t*;
    auto get_sort_modes() const -> const sort_modes_t&;
    auto update_panel_info(OpenPanelInfo *info) -> void;
protected:
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;
    auto process_key_input(int combined_key) -> intptr_t;

    virtual auto start_playback(const string &track_id) -> bool = 0;
    virtual auto get_tracks() -> std::generator<const simplified_track_t&> = 0;
protected:
    api_abstract *api_proxy;
};


/// @brief Showing the list of the given `album` tracks
class album_tracks_view:
    public tracks_base_view,
    public playback_observer
{
public:
    album_tracks_view(api_abstract *api, const album_t &album,
        return_callback_t callback);
    ~album_tracks_view();
    
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto start_playback(const string &track_id) -> bool;
    auto get_tracks() -> std::generator<const simplified_track_t&>;

    auto on_track_changed(const track_t &track) -> void;
private:
    album_t album;
    album_tracks_ptr collection;
};

/// @brief 
class recent_tracks_view:
    public tracks_base_view,
    public play_history_observer
{
public:
    struct history_track_t: public track_t
    {
        string played_at;

        history_track_t(const string &played_at, const track_t &track):
            track_t(track), played_at(played_at)
            {}
    };
public:
    recent_tracks_view(api_abstract *api);
    ~recent_tracks_view();
    
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
    auto get_sort_modes() const -> const view::sort_modes_t&;
protected:
    auto rebuild_items() -> void;

    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;
    auto start_playback(const string &track_id) -> bool;
    auto get_tracks() -> std::generator<const simplified_track_t&>;
    
    void on_items_changed();
private:
    std::vector<history_track_t> items;
};

/// @brief
class saved_tracks_view:
    public tracks_base_view,
    public playback_observer
{
public:
    saved_tracks_view(api_abstract *api);
    ~saved_tracks_view();
    
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto start_playback(const string &track_id) -> bool;
    auto get_tracks() -> std::generator<const simplified_track_t&>;

    auto on_track_changed(const track_t &track) -> void;
private:
    saved_tracks_ptr collection;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB