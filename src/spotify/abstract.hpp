#ifndef ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6

#include "utils.hpp"
#include "config.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

struct api_abstract
{
    virtual ~api_abstract() {}

    /// @brief Checks the spotify authorizations status
    virtual bool is_authenticated() const = 0;

    virtual void set_frequent_syncs(bool is_on) = 0;
    virtual bool is_frequent_syncs() const = 0;
    
    // library & collections interface
    virtual auto get_available_devices() -> const devices_t& = 0;
    virtual auto get_playback_state() -> const playback_state& = 0;
    virtual auto get_artist(const string &artist_id) -> artist  = 0;
    virtual auto get_artist_top_tracks(const string &artist_id) -> tracks_t = 0;
    virtual auto get_album(const string &album_id) -> album = 0;
    virtual auto get_album_tracks(const string &album_id) -> simplified_tracks_t = 0;
    virtual auto get_playlist(const string &playlist_id) -> playlist = 0;
    virtual auto get_playlist_tracks(const string &playlist_id) -> playlist_tracks_t = 0;
    virtual auto check_saved_track(const string &track_id) -> bool = 0;
    virtual auto check_saved_tracks(const std::vector<string> &ids) -> std::vector<bool> = 0;
    virtual auto save_tracks(const std::vector<string> &ids) -> bool = 0;
    virtual auto remove_saved_tracks(const std::vector<string> &ids) -> bool = 0;
    virtual auto get_playing_queue() -> playing_queue = 0;

    // playback interface
    virtual void start_playback(const string &context_uri, const string &track_uri = "",
                                int position_ms = 0, const string &device_id = "") = 0;
    virtual void start_playback(const std::vector<string> &uris, const string &device_id = "") = 0;
    virtual void start_playback(const simplified_album &album, const simplified_track &track) = 0;
    virtual void start_playback(const simplified_playlist &playlist, const simplified_track &track) = 0;
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

    /// @brief Performs a HTTP GET request
    /// @param cache_for caches the requested data for the givem amount of time
    virtual httplib::Result get(const string &request_url, utils::clock_t::duration cache_for = {}) = 0;
    virtual httplib::Result put(const string &request_url, const json &body = {}) = 0;
    virtual httplib::Result del(const string &request_url, const json &body = {}) = 0;
};

/// @brief An interface to the class, which implements the functionality to cache the data
/// and store it in the local storage
struct cached_data_abstract: public config::persistent_data_abstract
{
    /// @brief An method to resync data from the server
    /// @param force - if true, the data will be resynced regardless of the cache validity
    virtual void resync(bool force = false) = 0;

    /// @brief Return true if the cache should not be resynced
    virtual bool is_active() const { return true; }
};

} // namespace spotify
} // namespace spotifar

#endif // ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6