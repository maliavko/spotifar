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

    void clear_http_cache();
    bool is_request_cached(const string &url) const;

    bool is_authenticated() const { return auth->is_authenticated(); }
    
    // library api interface
    auto get_play_history() -> const history_items_t& { return history->get(); }
    auto get_available_devices() -> const devices_t& { return devices->get(); }
    auto get_playback_state() -> const playback_state_t& { return playback->get(); }
    auto get_followed_artists() -> followed_artists_ptr;
    auto get_artist(const string &artist_id) -> artist_t;
    auto get_artists(const item_ids_t &ids) -> std::vector<artist_t>;
    auto get_artist_albums(const string &artist_id) -> artist_albums_ptr;
    auto get_saved_albums() -> saved_albums_ptr;
    auto get_new_releases() -> new_releases_ptr;
    auto get_artist_top_tracks(const string &artist_id) -> tracks_t;
    auto get_album(const string &album_id) -> album_t;
    auto get_albums(const item_ids_t &ids) -> std::vector<album_t>;
    auto get_album_tracks(const string &album_id) -> album_tracks_ptr;
    auto get_playlist(const string &playlist_id) -> playlist_t;
    auto get_playlists() -> const simplified_playlists_t&;
    auto get_playlist_tracks(const string &playlist_id) -> const saved_tracks_t&;
    auto check_saved_track(const string &track_id) -> bool;
    auto check_saved_tracks(const item_ids_t &ids) -> std::vector<bool>;
    auto save_tracks(const item_ids_t &ids) -> bool;
    auto remove_saved_tracks(const item_ids_t &ids) -> bool;
    auto get_playing_queue() -> playing_queue_t;
    auto get_recently_played(std::int64_t after) -> recently_played_tracks_ptr;

    // playback api interface
    void start_playback(const string &context_uri, const string &track_uri = "",
        int position_ms = 0, const string &device_id = "");
    void start_playback(const std::vector<string> &uris, const string &device_id = "");
    void start_playback(const simplified_album_t &album, const simplified_track_t &track);
    void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track);
    void resume_playback(const string &device_id = "");
    void toggle_playback(const string &device_id = "");
    void pause_playback(const string &device_id = "");
    void skip_to_next(const string &device_id = "");
    void skip_to_previous(const string &device_id = "");
    void seek_to_position(int position_ms, const string &device_id = "");
    void toggle_shuffle(bool is_on, const string &device_id = "");
    void toggle_shuffle_plus(bool is_on);
    void set_repeat_state(const string &mode, const string &device_id = "");
    void set_playback_volume(int volume_percent, const string &device_id = "");
    void transfer_playback(const string &device_id, bool start_playing = false);

protected:
    void start_playback(const json &body, const string &device_id);
    
    // the main interface for raw http requests
    Result get(const string &url, utils::clock_t::duration cache_for = {});
    Result put(const string &url, const json &body = {});
    Result del(const string &url, const json &body = {});
    Result post(const string &url, const json &body = {});
    
    BS::thread_pool& get_pool() override { return pool; };

    std::shared_ptr<httplib::Client> get_client() const;

    /// @brief a helpers function for getting one item from API
    template<class R, typename... ArgumentsTypes>
    auto get_item(ArgumentsTypes... args) -> typename R::value_t;

    /// @brief a helpers function for getting one item from API
    template<class R, typename... ArgumentsTypes>
    auto get_several_items(ArgumentsTypes... args) -> const typename R::value_t&;

    /// @brief a helpers function for getting collection items from API
    template<class R, typename... ArgumentsTypes>
    auto get_items_collection(ArgumentsTypes... args) -> const typename R::value_t&;
    
    void on_auth_status_changed(const auth_t &auth); // auth status listener

private:
    BS::thread_pool pool;
    httplib::Client client;
    http_cache api_responses_cache;

    // caches
    std::unique_ptr<playback_cache> playback;
    std::unique_ptr<devices_cache> devices;
    std::unique_ptr<auth_cache> auth;
    std::unique_ptr<play_history> history;

    std::vector<cached_data_abstract*> caches;
};

template<class R, typename... ArgumentsTypes>
auto api::get_item(ArgumentsTypes... args) -> typename R::value_t
{
    auto requester = R(args...);
    if (requester(this))
        return requester.get();
    return {};
}

template<class R, typename... ArgumentsTypes>
auto api::get_several_items(ArgumentsTypes... args) -> const typename R::value_t&
{
    // keeping static container with data for being able to return a reference
    static typename R::value_t result;
    result.clear();

    // accumulating chunked results into one container
    auto requester = R(args...);
    for (const auto &entries: requester.fetch_by_chunks(this))
        result.insert(result.end(), entries.begin(), entries.end());

    return result;
}

template<class R, typename... ArgumentsTypes>
auto api::get_items_collection(ArgumentsTypes... args) -> const typename R::value_t&
{
    // keeping static container with data for being able to return a reference
    static typename R::value_t result;
    result.clear();

    // accumulating paged results into one container
    auto requester = R(args...);
    for (const auto &entries: requester.fetch_by_pages(this))
    {
        result.insert(result.end(), entries.begin(), entries.end());
        //break; // TODO: remove! just for speeding up the testing
    }

    return result;
}

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66