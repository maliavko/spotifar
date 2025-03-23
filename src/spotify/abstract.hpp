#ifndef ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#pragma once

#include "utils.hpp"
#include "config.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

using httplib::Result;

struct api_abstract
{
    template<class T> friend class collection;

    virtual ~api_abstract() {}

    /// @brief Checks the spotify authorizations status
    virtual bool is_authenticated() const = 0;
    virtual bool is_request_cached(const string &url) const = 0;
    
    // library & collections interface
    virtual auto get_play_history() -> const history_items_t& = 0;
    virtual auto get_available_devices() -> const devices_t& = 0;
    virtual auto get_playback_state() -> const playback_state_t& = 0;
    virtual auto get_followed_artists() -> const artists_t& = 0;
    virtual auto get_artist(const string &artist_id) -> artist_t  = 0;
    virtual auto get_artists(const std::vector<string> &ids) -> const artists_t&  = 0;
    virtual auto get_artist_albums(const string &artist_id) -> const simplified_albums_t&  = 0;
    virtual auto get_saved_albums() -> const saved_albums_t& = 0;
    virtual auto get_new_releases() -> const simplified_albums_t& = 0;
    virtual auto get_artist_top_tracks(const string &artist_id) -> tracks_t = 0;
    virtual auto get_album(const string &album_id) -> album_t = 0;
    virtual auto get_albums(const std::vector<string> &ids) -> albums_t = 0;
    virtual auto get_album_tracks(const string &album_id) -> const simplified_tracks_t& = 0;
    virtual auto get_playlist(const string &playlist_id) -> playlist_t = 0;
    virtual auto get_playlists() -> const simplified_playlists_t& = 0;
    virtual auto get_playlist_tracks(const string &playlist_id) -> const saved_tracks_t& = 0;
    virtual auto check_saved_track(const string &track_id) -> bool = 0;
    virtual auto check_saved_tracks(const std::vector<string> &ids) -> std::vector<bool> = 0;
    virtual auto save_tracks(const std::vector<string> &ids) -> bool = 0;
    virtual auto remove_saved_tracks(const std::vector<string> &ids) -> bool = 0;
    virtual auto get_playing_queue() -> playing_queue_t = 0;
    virtual auto get_recently_played(std::int64_t after) -> const history_items_t& = 0;

    // playback interface
    virtual void start_playback(const string &context_uri, const string &track_uri = "",
                                int position_ms = 0, const string &device_id = "") = 0;
    virtual void start_playback(const std::vector<string> &uris, const string &device_id = "") = 0;
    virtual void start_playback(const simplified_album_t &album, const simplified_track_t &track) = 0;
    virtual void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track) = 0;
    virtual void resume_playback(const string &device_id = "") = 0;
    virtual void toggle_playback(const string &device_id = "") = 0;
    virtual void pause_playback(const string &device_id = "") = 0;
    virtual void skip_to_next(const string &device_id = "") = 0;
    virtual void skip_to_previous(const string &device_id = "") = 0;
    virtual void seek_to_position(int position_ms, const string &device_id = "") = 0;
    virtual void toggle_shuffle(bool is_on, const string &device_id = "") = 0;
    virtual void toggle_shuffle_plus(bool is_on) = 0;
    virtual void set_repeat_state(const string &mode, const string &device_id = "") = 0;
    virtual void set_playback_volume(int volume_percent, const string &device_id = "") = 0;
    virtual void transfer_playback(const string &device_id, bool start_playing = false) = 0;
    //TODO: return here protected pecifier
    /// @brief Performs an HTTP GET request
    /// @param cache_for caches the requested data for the given amount of time
    virtual Result get(const string &url, utils::clock_t::duration cache_for = {}) = 0;
    virtual Result put(const string &url, const json &body = {}) = 0;
    virtual Result del(const string &url, const json &body = {}) = 0;

    virtual BS::thread_pool& get_pool() = 0;
};

} // namespace spotify
} // namespace spotifar

#endif // ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6