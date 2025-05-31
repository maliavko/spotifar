#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/common.hpp"
#include "spotify/playback.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class tracks_base_view: public view_abstract
{
public:
    tracks_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, return_callback_t callback);
    ~tracks_base_view() { api_proxy.reset(); }

    auto get_items() -> const items_t& override;
protected:
    virtual bool start_playback(const string &track_id) = 0;
    virtual auto get_tracks() -> std::generator<const simplified_track_t&> = 0;

    // view interface
    void update_panel_info(OpenPanelInfo *info) override;
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t override;
    auto process_key_input(int combined_key) -> intptr_t override;
protected:
    api_weak_ptr_t api_proxy;
};

/// @brief Showing the list of the given `album` tracks
class album_tracks_view:
    public tracks_base_view,
    public playback_observer // `on_items_changed` to refresh panel
                             // when playing tracks is being changed
{
public:
    album_tracks_view(HANDLE panel, api_weak_ptr_t api, const simplified_album_t &album,
        return_callback_t callback);
    ~album_tracks_view();
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const simplified_track_t&> override;

    // playback_observer handlers
    void on_track_changed(const track_t &track, const track_t &prev_track) override;
private:
    album_t album;
    album_tracks_ptr collection;
};

/// @brief A class-view, representing a list of recently played tracks
class recent_tracks_view:
    public tracks_base_view,
    public play_history_observer // `on_items_changed` to refresh panel
{
public:
    struct history_track_t: public track_t
    {
        string played_at;
    };
public:
    recent_tracks_view(HANDLE panel, api_weak_ptr_t api);
    ~recent_tracks_view();
protected:
    void rebuild_items();

    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    auto get_sort_modes() const -> const view_abstract::sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const simplified_track_t&> override;
    
    // playback_observer handlers
    void on_items_changed();
private:
    std::vector<history_track_t> items;
};

/// @brief A class-view, representing a list of saved (liked) tracks
class saved_tracks_view:
    public tracks_base_view,
    public playback_observer // `on_items_changed`
{
public:
    saved_tracks_view(HANDLE panel, api_weak_ptr_t api);
    ~saved_tracks_view();
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const simplified_track_t&> override;

    // playback_observer handlers
    void on_track_changed(const track_t &track, const track_t &prev_track) override;
private:
    saved_tracks_ptr collection;
};

/// @brief A class-view, to represent on the panels a list of
/// the playing queue tracks
class playing_queue_view:
    public tracks_base_view,
    public playback_observer // on_items_changed, on_shuffle_state_changed
{
public:
    playing_queue_view(HANDLE panel, api_weak_ptr_t api);
    ~playing_queue_view();
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const simplified_track_t&> override;

    // playback_observer handlers
    void on_track_changed(const track_t &track, const spotify::track_t &prev_track) override;
    void on_shuffle_state_changed(bool shuffle_state) override;
};


/// @brief
class recently_liked_tracks_view: public tracks_base_view
{
public:
    recently_liked_tracks_view(HANDLE panel, api_weak_ptr_t api);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const simplified_track_t&> override;
private:
    saved_tracks_ptr collection;
};


/// @brief
class user_top_tracks_view: public tracks_base_view
{
public:
    user_top_tracks_view(HANDLE panel, api_weak_ptr_t api);
protected:
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const simplified_track_t&> override;
private:
    user_top_tracks_ptr collection;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB