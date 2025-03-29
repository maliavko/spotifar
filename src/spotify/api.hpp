#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "abstract.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "auth.hpp"
#include "history.hpp"

namespace spotifar { namespace spotify {

class api:
    public api_abstract,
    public auth_observer
{
public:
    api();
    ~api();

    bool start();
    void shutdown();
    void tick();

    bool is_authenticated() const { return auth->is_authenticated(); }

    auto get_play_history() -> const history_items_t& { return history->get(); }
    auto get_available_devices() -> const devices_t& { return devices->get(); }
    auto get_playback_state() -> const playback_state_t& { return playback->get(); }
    
    // library api interface
    auto get_followed_artists() -> followed_artists_ptr;
    auto get_artist(const item_id_t &artist_id) -> artist_t;
    auto get_artists(const item_ids_t &ids) -> std::vector<artist_t>;
    auto get_artist_albums(const item_id_t &artist_id) -> artist_albums_ptr;
    auto get_saved_albums() -> saved_albums_ptr;
    auto get_new_releases() -> new_releases_ptr;
    auto get_artist_top_tracks(const item_id_t &artist_id) -> std::vector<track_t>;
    auto get_album(const item_id_t &album_id) -> album_t;
    auto get_albums(const item_ids_t &ids) -> std::vector<album_t>;
    auto get_album_tracks(const item_id_t &album_id) -> album_tracks_ptr;
    auto get_playlist(const item_id_t &playlist_id) -> playlist_t;
    auto get_saved_playlists() -> saved_playlists_ptr;
    auto get_playlist_tracks(const item_id_t &playlist_id) -> saved_tracks_ptr;
    auto check_saved_track(const item_id_t &track_id) -> bool;
    auto check_saved_tracks(const item_ids_t &ids) -> std::deque<bool>;
    auto save_tracks(const item_ids_t &ids) -> bool;
    auto remove_saved_tracks(const item_ids_t &ids) -> bool;
    auto get_playing_queue() -> playing_queue_t;
    auto get_saved_tracks() -> saved_tracks_ptr;

    // playback api interface
    void start_playback(const string &context_uri, const string &track_uri = "",
        int position_ms = 0, const item_id_t &device_id = "");
    void start_playback(const std::vector<string> &uris, const item_id_t &device_id = "");
    void start_playback(const simplified_album_t &album, const simplified_track_t &track);
    void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track);
    void resume_playback(const item_id_t &device_id = "");
    void toggle_playback(const item_id_t &device_id = "");
    void pause_playback(const item_id_t &device_id = "");
    void skip_to_next(const item_id_t &device_id = "");
    void skip_to_previous(const item_id_t &device_id = "");
    void seek_to_position(int position_ms, const item_id_t &device_id = "");
    void toggle_shuffle(bool is_on, const item_id_t &device_id = "");
    void toggle_shuffle_plus(bool is_on);
    void set_repeat_state(const string &mode, const item_id_t &device_id = "");
    void set_playback_volume(int volume_percent, const item_id_t &device_id = "");
    void transfer_playback(const item_id_t &device_id, bool start_playing = false);
protected:
    /// @brief Creates a new http-client instance, fills up all
    /// default attributes and token, and returns it 
    auto get_client() const -> std::shared_ptr<httplib::Client>;
    
    void start_playback_raw(const string &body, const string &device_id);
    
    // the main interface for raw http requests
    Result get(const string &url, utils::clock_t::duration cache_for = {}) override;
    Result put(const string &url, const string &body = "") override;
    Result del(const string &url, const string &body = "") override;
    Result post(const string &url, const string &body = "") override;
    
    auto get_pool() -> BS::thread_pool& override { return pool; };
    auto is_request_cached(const string &url) const -> bool override;
    
    void on_auth_status_changed(const auth_t &auth) override; // auth status listener
private:
    BS::thread_pool pool;
    http_cache api_responses_cache;

    // caches
    std::unique_ptr<playback_cache> playback;
    std::unique_ptr<devices_cache> devices;
    std::unique_ptr<auth_cache> auth;
    std::unique_ptr<play_history> history;

    std::vector<cached_data_abstract*> caches;
};

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66