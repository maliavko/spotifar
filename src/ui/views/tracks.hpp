#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/common.hpp"
#include "spotify/playback.hpp"
#include "spotify/history.hpp"
#include "spotify/collection.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class tracks_base_view:
    public view,
    public collection_observer
{
public:
    tracks_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name = L"");
    ~tracks_base_view();

    auto get_items() -> const items_t& override;
protected:
    virtual bool start_playback(const item_id_t &track_id) = 0;
    virtual auto get_tracks() -> std::generator<const track_t&> = 0;
    virtual auto get_extra_columns(const track_t&) const -> std::vector<wstring> { return {}; }

    // view
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
    auto process_key_input(int combined_key) -> intptr_t override;
    auto get_key_bar_info() -> const key_bar_info_t* override;
    auto get_panel_modes() const -> const panel_modes_t* override;

    // collection_observer
    void on_saved_tracks_changed(const item_ids_t &ids) override;
protected:
    api_weak_ptr_t api_proxy;
    items_t items;
};


/// @brief Showing the list of tracks of the given `album`
class album_tracks_view:
    public tracks_base_view,
    public playback_observer
{
public:
    album_tracks_view(HANDLE panel, api_weak_ptr_t api, const simplified_album_t &album);
    ~album_tracks_view();
protected:
    void rebuild_items();

    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const track_t&> override;
    auto get_extra_columns(const track_t&) const -> std::vector<wstring> override;
    auto get_panel_modes() const -> const panel_modes_t* override { return &panel_modes; }

    // playback_observer handlers
    void on_track_changed(const track_t &track, const track_t &prev_track) override;
private:
    bool is_multidisc = false;
    album_t album;
    std::vector<track_t> album_tracks;
    panel_modes_t panel_modes;
};


/// @brief Recently played tracks view
class recent_tracks_view:
    public tracks_base_view,
    public play_history_observer
{
    // extending a track_t class with `played_at` field, used only
    // locally in the view class
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
    auto get_sort_modes() const -> const view::sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const track_t&> override;
    auto get_extra_columns(const track_t& track) const -> std::vector<wstring> override;
    auto get_panel_modes() const -> const panel_modes_t* override { return &panel_modes; }
    
    // play_history_observer handlers
    void on_items_changed();
private:
    std::vector<history_track_t> items;
    panel_modes_t panel_modes;
};


/// @brief List of saved (liked) tracks, so-called user collection. 
class saved_tracks_view:
    public tracks_base_view,
    public playback_observer
{
public:
    saved_tracks_view(HANDLE panel, api_weak_ptr_t api);
    ~saved_tracks_view();
protected:
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const track_t&> override;
    auto get_extra_columns(const track_t&) const -> std::vector<wstring> override;
    auto get_panel_modes() const -> const panel_modes_t* override { return &panel_modes; }

    // playback_observer handlers
    void on_track_changed(const track_t &track, const track_t &prev_track) override;
private:
    saved_tracks_ptr collection;
    panel_modes_t panel_modes;
};


/// @brief A currently playing queue view
class playing_queue_view:
    public tracks_base_view,
    public playback_observer
{
public:
    playing_queue_view(HANDLE panel, api_weak_ptr_t api);
    ~playing_queue_view();
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    auto get_sort_modes() const -> const sort_modes_t& override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const track_t&> override;

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
    auto get_tracks() -> std::generator<const track_t&> override;
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
    auto get_tracks() -> std::generator<const track_t&> override;
private:
    user_top_tracks_ptr collection;
};


/// @brief
class artist_top_tracks_view:
    public tracks_base_view,
    public playback_observer
{
public:
    artist_top_tracks_view(HANDLE panel, api_weak_ptr_t api, const artist_t &artist);
    ~artist_top_tracks_view();
protected:
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;

    // tracks_base_view interface
    bool start_playback(const string &track_id) override;
    auto get_tracks() -> std::generator<const track_t&> override;

    // playback_observer handlers
    void on_track_changed(const track_t &track, const spotify::track_t &prev_track) override;
    void on_shuffle_state_changed(bool shuffle_state) override;
private:
    std::vector<track_t> tracks;
    artist_t artist;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB