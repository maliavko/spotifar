#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "interfaces.hpp"

namespace spotifar { namespace spotify {

/// @brief A class-helper to guard an endpoint from spam requests. Implemets the logic
/// to track endpoint's busy (retry after) time and blocks accessing threads until the
/// delay is expired. The algorithm is built on the conditional variables and mutexes:
/// API receiving 429 http error marks an endpoint as busy and all the further requests
/// can check its status to avoid spamming or get into the waiting queue until it is free
///
/// @note The endpoint's name is the first part of the requesting url, except API version
/// specificator: /v1/me/player/ -> "me", /v1/artists/123kjasd2342 -> artists
class endpoint_guard
{
public:
    endpoint_guard(const string &name): name(name) {}

    /// @brief Blocks an accessing thread until the delay time is expired or
    /// `predicate` returns `true`
    void wait(std::function<bool()> predicate);

    /// @brief Set the busy expiration time
    void set_expires_at(const utils::clock_t::time_point &);

    /// @brief Returns the busy status expiration time point
    auto get_expires_at() const -> const utils::clock_t::time_point&;

    /// @brief Returns the endpoint's busy status, if it is rate limited and should be not used
    /// until the expiration time
    bool is_rate_limited() const { return expires_at > utils::clock_t::now(); }

    /// @brief Notifies all the blocked (pending) threads to wake up and check their statuses
    void notify_all() { cv.notify_all(); }

    /// @brief The endpoint's name
    auto get_name() const -> const string& { return name; }
private:
    std::condition_variable cv;
    std::mutex guard;
    utils::clock_t::time_point expires_at{};
    string name;
};


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
    
    auto get_ptr() -> api_weak_ptr_t override { return shared_from_this(); }

    auto get_play_history(bool force_resync = false) -> const history_items_t& override;
    auto get_playback_state(bool force_resync = false) -> const playback_state_t& override;

    auto get_library() -> library_interface* override;
    auto get_releases() -> recent_releases_interface* override;
    auto get_auth_cache() -> auth_cache_interface* override;
    auto get_devices_cache(bool resync = false) -> devices_cache_interface* override;
    
    // library api interface

    auto get_artist(const item_id_t &artist_id) -> artist_t override;
    auto get_artists(const item_ids_t &ids) -> std::vector<artist_t> override;
    auto get_artist_albums(const item_id_t &artist_id, const std::vector<string> &groups) -> artist_albums_ptr override;
    auto get_artist_top_tracks(const item_id_t &artist_id) -> std::vector<track_t> override;
    auto get_album(const item_id_t &album_id) -> album_t override;
    auto get_albums(const item_ids_t &ids) -> std::vector<album_t> override;
    auto get_tracks(const item_ids_t &ids) -> std::vector<track_t> override;
    auto get_album_tracks(const item_id_t &album_id) -> album_tracks_ptr override;
    auto get_playlist(const item_id_t &playlist_id) -> playlist_t override;
    auto get_user_top_tracks() -> user_top_tracks_ptr override;
    auto get_user_top_artists() -> user_top_artists_ptr override;
    auto get_saved_playlists() -> saved_playlists_ptr override;
    auto get_playlist_tracks(const item_id_t &playlist_id) -> saved_tracks_ptr override;
    auto get_playing_queue() -> playing_queue_t override;
    auto get_image(const image_t &image, const item_id_t &item_id) -> wstring override;
    auto get_lyrics(const track_t &) -> string override;

    // playback api interface

    void start_playback(const string &context_uri, const string &track_uri = "", int position_ms = 0, const item_id_t &device_id = "") override;
    void start_playback(const std::vector<string> &uris, const string &track_uri = "", const item_id_t &device_id = "") override;
    void start_playback(const simplified_album_t &album, const simplified_track_t &track) override;
    void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track) override;
    void resume_playback(const item_id_t &device_id = "") override;
    void pause_playback(const item_id_t &device_id = "") override;
    void skip_to_next(const item_id_t &device_id = "") override;
    void skip_to_previous(const item_id_t &device_id = "") override;
    void seek_to_position(int position_ms, const item_id_t &device_id = "") override;
    void toggle_shuffle(bool is_on, const item_id_t &device_id = "") override;
    void set_repeat_state(const string &mode, const item_id_t &device_id = "") override;
    void set_playback_volume(int volume_percent, const item_id_t &device_id = "") override;
protected:
    /// @brief Creates a new http-client instance with the Spotify web API domain address,
    /// fills up all the default attributes and token, and returns it
    auto get_client() const -> std::shared_ptr<httplib::Client>;

    /// @brief Returns the endpoint guard, depending on the given `url`
    auto get_endpoint(const string &url) -> endpoint_guard&;
    
    /// @brief A raw low level start-playback method, receives a http POST request json body string,
    /// in the certain format
    ///
    /// https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
    void start_playback_base(const string &body, const item_id_t &device_id);
    
    auto get(const string &url, utils::clock_t::duration cache_for = {}, bool retry_429 = false) -> httplib::Result override;
    auto put(const string &url, const string &body = "") -> httplib::Result override;
    auto del(const string &url, const string &body = "") -> httplib::Result override;
    auto post(const string &url, const string &body = "") -> httplib::Result override;
    
    auto get_pool() -> BS::light_thread_pool& override { return requests_pool; };
    bool is_request_cached(const string &url) const override;
    bool is_endpoint_rate_limited(const string &endpoint_name) const override;
    void cancel_pending_requests(bool wait_for_result = true) override;
private:
    BS::light_thread_pool requests_pool;
    BS::light_thread_pool resyncs_pool;

    std::unordered_map<string, endpoint_guard> guards;

    /// @brief A pending requests cancellation flag, used by `cancel_pending_requests` method
    bool cancel_flag = false;

    // caches

    std::unique_ptr<http_cache> api_responses_cache;

    std::unique_ptr<library> library;
    std::unique_ptr<playback_cache> playback;
    std::unique_ptr<devices_cache> devices;
    std::unique_ptr<auth_cache> auth;
    std::unique_ptr<play_history> history;
    std::unique_ptr<recent_releases> releases;

    std::vector<cached_data_abstract*> caches;
};

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66