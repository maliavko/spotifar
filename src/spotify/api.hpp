#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "common.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "auth.hpp"
#include "history.hpp"

namespace spotifar { namespace spotify {

class api:
    public api_interface,
    public std::enable_shared_from_this<api>
{
public:
    api();
    ~api();

    bool start();
    void shutdown();
    void tick();

    bool is_authenticated() const  override { return auth->is_authenticated(); }
    
    auto get_ptr() -> api_proxy_ptr override { return shared_from_this(); }

    auto get_access_token() -> const string& override { return auth->get_access_token(); }
    auto get_play_history(bool force_resync = false) -> const history_items_t& override;
    auto get_available_devices(bool force_resync = false) -> const devices_t& override;
    auto get_playback_state(bool force_resync = false) -> const playback_state_t& override;
    
    // library api interface
    auto get_followed_artists() -> followed_artists_ptr override;
    auto get_artist(const item_id_t &artist_id) -> artist_t override;
    auto get_artists(const item_ids_t &ids) -> std::vector<artist_t> override;
    auto get_artist_albums(const item_id_t &artist_id) -> artist_albums_ptr override;
    auto get_saved_albums() -> saved_albums_ptr override;
    auto get_new_releases() -> new_releases_ptr override;
    auto get_artist_top_tracks(const item_id_t &artist_id) -> std::vector<track_t> override;
    auto get_album(const item_id_t &album_id) -> album_t override;
    auto get_albums(const item_ids_t &ids) -> std::vector<album_t> override;
    auto get_album_tracks(const item_id_t &album_id) -> album_tracks_ptr override;
    auto get_playlist(const item_id_t &playlist_id) -> playlist_t override;
    auto get_saved_playlists() -> saved_playlists_ptr override;
    auto get_playlist_tracks(const item_id_t &playlist_id) -> saved_tracks_ptr override;
    auto check_saved_track(const item_id_t &track_id) -> bool override;
    auto check_saved_tracks(const item_ids_t &ids) -> std::deque<bool> override;
    auto save_tracks(const item_ids_t &ids) -> bool override;
    auto remove_saved_tracks(const item_ids_t &ids) -> bool override;
    auto get_playing_queue() -> playing_queue_t override;
    auto get_saved_tracks() -> saved_tracks_ptr override;
    auto get_image(const image_t &image, const item_id_t &item_id) -> wstring override;

    // playback api interface
    void start_playback(const string &context_uri, const string &track_uri = "",
        int position_ms = 0, const item_id_t &device_id = "") override;
    void start_playback(const std::vector<string> &uris, const item_id_t &device_id = "") override;
    void start_playback(const simplified_album_t &album, const simplified_track_t &track) override;
    void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track) override;
    void resume_playback(const item_id_t &device_id = "") override;
    void toggle_playback(const item_id_t &device_id = "") override;
    void pause_playback(const item_id_t &device_id = "") override;
    void skip_to_next(const item_id_t &device_id = "") override;
    void skip_to_previous(const item_id_t &device_id = "") override;
    void seek_to_position(int position_ms, const item_id_t &device_id = "") override;
    void toggle_shuffle(bool is_on, const item_id_t &device_id = "") override;
    void set_repeat_state(const string &mode, const item_id_t &device_id = "") override;
    void set_playback_volume(int volume_percent, const item_id_t &device_id = "") override;
    void transfer_playback(const item_id_t &device_id, bool start_playing = false) override;
protected:
    /// @brief Creates a new http-client instance with the Spotify web API domain address,
    /// fills up all the default attributes and token, and returns it
    auto get_client() const -> std::shared_ptr<httplib::Client>;
    
    void start_playback_raw(const string &body, const item_id_t &device_id);
    
    // the main interface for raw http requests
    Result get(const string &url, utils::clock_t::duration cache_for = {}) override;
    Result put(const string &url, const string &body = "") override;
    Result del(const string &url, const string &body = "") override;
    Result post(const string &url, const string &body = "") override;
    
    auto get_pool() -> BS::thread_pool& override { return pool; };
    bool is_request_cached(const string &url) const override;
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

using api_ptr = std::shared_ptr<api>;

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66