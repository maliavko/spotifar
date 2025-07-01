#ifndef INTERFACES_HPP_F3709CC3_8A2C_4B4F_97B7_C51257CA1959
#define INTERFACES_HPP_F3709CC3_8A2C_4B4F_97B7_C51257CA1959
#pragma once

#include "stdafx.h"
#include "items.hpp"

namespace spotifar { namespace spotify {

template<class T, int N = 0, class C = utils::clock_t::duration>
class item_requester;

template<class T, int N, class C, class ContainerT = std::vector<typename T>>
class several_items_requester;

template<class T, int N = 0, class C = utils::clock_t::duration>
class sync_collection;

template<class T, int N = 0, class C = utils::clock_t::duration>
class async_collection;


using followed_artists_t = sync_collection<artist_t, -1>;
using followed_artists_ptr = std::shared_ptr<followed_artists_t>;

using saved_albums_t = async_collection<saved_album_t, 1, std::chrono::days>;
using saved_albums_ptr = std::shared_ptr<saved_albums_t>;

using saved_tracks_t = async_collection<saved_track_t, 1, std::chrono::days>;
using saved_tracks_ptr = std::shared_ptr<saved_tracks_t>;

using saved_playlists_t = async_collection<simplified_playlist_t, -1>;
using saved_playlists_ptr = std::shared_ptr<saved_playlists_t>;

using artist_albums_t = async_collection<simplified_album_t, 3, std::chrono::days>;
using artist_albums_ptr = std::shared_ptr<artist_albums_t>;

using album_tracks_t = async_collection<simplified_track_t, 1, std::chrono::months>;
using album_tracks_ptr = std::shared_ptr<album_tracks_t>;

using user_top_tracks_t = async_collection<track_t, 1, std::chrono::weeks>;
using user_top_tracks_ptr = std::shared_ptr<user_top_tracks_t>;

using user_top_artists_t = async_collection<artist_t, 1, std::chrono::weeks>;
using user_top_artists_ptr = std::shared_ptr<user_top_artists_t>;


struct collection_interface
{
    /// @brief Returns the total count of items in the collection, performs a single
    /// server request if needed silently
    virtual size_t get_total() const = 0;

    /// @brief Returns the total count of items in the collection if available or zero,
    /// works only with cache, does not perform any request
    virtual size_t peek_total() const = 0;

    /// @brief Tells whether the collection was requested before and successfully cached.
    /// @note checks only the first request in the collection fetching sequence, possibly faulty
    /// in case only some of them have been cached
    virtual bool is_cached() const = 0;
    
    /// @brief Returns a flag, whether the collection was modified in the contrary to
    /// the cached one, and was resynced successfully
    virtual bool is_modified() const = 0;

    /// @brief A public interface method to populate the collection fully
    /// @param only_cached flag, telling the logic, that the method wa called
    /// from some heavy environment and should not perform many http calls
    /// @param notify_watchers does not send changes to the requesting status observers like
    /// showing request progress splashing screen and etc.
    /// @param pages_to_request number of data pages to request; "0" means all
    virtual bool fetch(bool only_cached = false, bool notify_watchers = true, size_t pages_to_request = 0) = 0;

    /// @brief Returns whether the container is populated from server or not
    virtual bool is_populated() const = 0;
};


struct library_interface
{
    friend class saved_tracks_collection;
    friend class saved_albums_collection;
    friend class followed_artists_collection;
public:
    /// @brief Checks the given track `id` saving status. Returns immediately if is cached,
    /// otherwise returns `false` and puts to the queue for requesting.
    /// @param force_sync forces method to check the status synchroniously
    /// 
    /// https://developer.spotify.com/documentation/web-api/reference/check-users-saved-tracks 
    virtual bool is_track_saved(const item_id_t &, bool force_sync = false) = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/save-tracks-user
    virtual bool save_tracks(const item_ids_t &) = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/remove-tracks-user
    virtual bool remove_saved_tracks(const item_ids_t &) = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
    virtual auto get_saved_tracks() -> saved_tracks_ptr = 0;

    /// @brief Checks the given album `id` saving status. Returns immediately if is cached,
    /// otherwise returns `false` and puts to the queue for requesting.
    /// @param force_sync forces method to check the status synchroniously
    /// 
    /// @brief https://developer.spotify.com/documentation/web-api/reference/check-users-saved-albums
    virtual bool is_album_saved(const item_id_t &, bool force_sync = false) = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/save-albums-user 
    virtual bool save_albums(const item_ids_t &) = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/remove-albums-user
    virtual bool remove_saved_albums(const item_ids_t &ds) = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-albums
    virtual auto get_saved_albums() -> saved_albums_ptr = 0;

    /// @brief Checks the given album `id` saving status. Returns immediately if is cached,
    /// otherwise returns `false` and puts to the queue for requesting.
    /// @param force_sync forces method to check the status synchroniously
    /// 
    /// @brief https://developer.spotify.com/documentation/web-api/reference/check-current-user-follows
    virtual bool is_artist_followed(const item_id_t &, bool force_sync = false) = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/follow-artists-users
    virtual bool follow_artists(const item_ids_t &) = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/unfollow-artists-users
    virtual bool unfollow_artists(const item_ids_t &) = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-followed
    virtual auto get_followed_artists() -> followed_artists_ptr = 0;
};


struct recent_releases_interface
{
    virtual bool is_cache_running() const = 0;

    /// @brief Amount of sync tasks in the queue, 0 - sync is finished 
    virtual auto get_sync_tasks_left() const -> size_t = 0;

    /// @brief List of currently cached fresh releases, `force_resync` forces
    /// the manager to resync data with server
    virtual auto get_items(bool force_resync = false) -> const recent_releases_t& = 0;

    /// @brief Invalidates the cache, automatically scheduling resync
    virtual void invalidate() = 0;

    /// @brief Returns the next sync time
    virtual auto get_next_sync_time() const -> const utils::clock_t::time_point = 0;
};


struct auth_cache_interface
{
    /// @brief Is successfully authenticated to Spotify API
    virtual bool is_authenticated() const = 0;

    /// @brief Returns a current session access token string
    virtual auto get_access_token() const -> const string& = 0;

    /// @brief Returns a current session refresh token string
    virtual auto get_refresh_token() const -> const string& = 0;

    /// @brief Clears current credentials and deletes caches as well,
    /// however does not break current session
    virtual void clear_credentials() = 0;
};


struct devices_cache_interface
{
    /// @brief Returns the list of all available currently devices
    virtual auto get_all() const -> const devices_t& = 0;

    /// @brief Returns a currently active device if any
    virtual auto get_active_device() const -> const device_t* = 0;

    /// @brief Returns a device by given id or nullptr
    virtual auto get_device_by_id(const item_id_t&) const -> const device_t* = 0;

    /// @brief Returns a device by given name or nullptr
    virtual auto get_device_by_name(const wstring&) const -> const device_t* = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/transfer-a-users-playback
    virtual void transfer_playback(const item_id_t &device_id, bool start_playing = false) = 0;
};


struct api_interface
{
    virtual ~api_interface() {}

    /// @brief A public interface for obtaining a weak pointer to the API interface
    /// instance. Used in many helper classes, avoiding passing a direct pointer for safety reasons
    virtual auto get_ptr() -> std::weak_ptr<api_interface> = 0;

    /// @brief Returns a played history list of items. If `force_resync` is true, the data
    /// is forcibly resynced before it is returned
    virtual auto get_play_history(bool force_resync = false) -> const history_items_t& = 0;

    /// @brief Returns a currently playing state object. If `force_resync` is true, the data
    /// is forcibly resynced before it is returned
    virtual auto get_playback_state(bool force_resync = false) -> const playback_state_t& = 0;

    /// @brief Returns a collections library interface for changing user's saved items:
    /// artists, albums or tracks
    virtual auto get_library() -> library_interface* = 0;
    
    /// @brief Returns a fresh releases management interface: get, invalidate etc.
    virtual auto get_releases() -> recent_releases_interface* = 0;

    /// @brief An interface for managing authentication cache
    virtual auto get_auth_cache() -> auth_cache_interface* = 0;

    /// @brief An interface for managing devices cache
    /// @param resync forces cache to get resynced beforehand
    virtual auto get_devices_cache(bool resync = false) -> devices_cache_interface* = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-top-tracks
    virtual auto get_artist_top_tracks(const item_id_t &artist_id) -> std::vector<track_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artist
    virtual auto get_artist(const item_id_t &artist_id) -> artist_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-artists
    virtual auto get_artists(const item_ids_t &ids) -> std::vector<artist_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-albums
    /// @param groups vector of strings [album, single, appears_on, compilation]
    virtual auto get_artist_albums(const item_id_t &artist_id, const std::vector<string> &groups) -> artist_albums_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-album
    virtual auto get_album(const item_id_t &album_id) -> album_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-albums
    virtual auto get_albums(const item_ids_t &ids) -> std::vector<album_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-several-tracks
    virtual auto get_tracks(const item_ids_t &ids) -> std::vector<track_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-albums-tracks
    virtual auto get_album_tracks(const item_id_t &album_id) -> album_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-playlists-tracks
    virtual auto get_playlist_tracks(const item_id_t &playlist_id) -> saved_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-list-users-playlists
    virtual auto get_saved_playlists() -> saved_playlists_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-playlist
    virtual auto get_playlist(const item_id_t &playlist_id) -> playlist_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-top-artists-and-tracks
    virtual auto get_user_top_tracks() -> user_top_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-top-artists-and-tracks
    virtual auto get_user_top_artists() -> user_top_artists_ptr = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-queue
    virtual auto get_playing_queue() -> playing_queue_t = 0;
    
    /// @brief Returns the downloaded and cached image filepath in case of success or empty string.
    /// @param image the image_t object to fetch from the Spotify server
    /// @param item_id the id of the item the image belongs to (e.g. album id or artist id)
    virtual auto get_image(const image_t &image, const item_id_t &item_id) -> wstring = 0;

    virtual auto get_lyrics(const track_t &) -> string = 0;

    // playback interface

    /// @brief Starts playback of a given `context_uri` context. If the `track_uri` is not empty,
    /// then starts given context from the specified track. If the `position_ms` is not 0,
    /// the given track is being started from the specified position in milliseconds
    virtual void start_playback(const string &context_uri, const string &track_uri = "",
        int position_ms = 0, const item_id_t &device_id = "") = 0;

    /// @brief Stars playback of a given list of tracks, provided via spotify URIs
    virtual void start_playback(const std::vector<string> &uris, const string &track_uri = "", const item_id_t &device_id = "") = 0;

    /// @brief Starts playback of the given `album` from the given `track` if provided
    virtual void start_playback(const simplified_album_t &album, const simplified_track_t &track) = 0;

    /// @brief Starts playback of the given `playlist` from the given `track` if provided
    virtual void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track) = 0;

    /// @brief Resumes suspended playback
    virtual void resume_playback(const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/pause-a-users-playback
    virtual void pause_playback(const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/skip-users-playback-to-next-track
    virtual void skip_to_next(const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/skip-users-playback-to-previous-track
    virtual void skip_to_previous(const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/seek-to-position-in-currently-playing-track
    virtual void seek_to_position(int position_ms, const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/toggle-shuffle-for-users-playback
    virtual void toggle_shuffle(bool is_on, const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/set-repeat-mode-on-users-playback
    /// @param mode string, either `track`, `context` or `off`
    virtual void set_repeat_state(const string &mode, const item_id_t &device_id = "") = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/set-volume-for-users-playback
    virtual void set_playback_volume(int volume_percent, const item_id_t &device_id = "") = 0;
//protected:
    /// @brief Performs an HTTP GET request
    /// @param cache_for caches the response for the given amount of time
    virtual httplib::Result get(const string &url, utils::clock_t::duration cache_for = {}, bool retry_429 = false) = 0;

    /// @brief Performs an HTTP PUT request
    virtual httplib::Result put(const string &url, const string &body = {}) = 0;

    /// @brief Performs an HTTP DEL request
    virtual httplib::Result del(const string &url, const string &body = {}) = 0;

    /// @brief Performs an HTTP POST request
    virtual httplib::Result post(const string &url, const string &body = {}) = 0;

    /// @brief Returns a reference to the internally allocated thread-pool. Used by
    /// requesters to perform async request
    virtual auto get_pool() -> BS::light_thread_pool& = 0;

    /// @brief Whether the given url is cached
    virtual bool is_request_cached(const string &url) const = 0;

private:
    friend class put_requester;

    friend class del_requester;

    template<class T, int N, class C>
    friend class item_requester;

    template<class T, int N, class C>
    friend class sync_collection;

    template<class T, int N, class C>
    friend class async_collection;

    friend class search_requester;
};

} // namespace spotify
} // namespace spotifar

#endif // INTERFACES_HPP_F3709CC3_8A2C_4B4F_97B7_C51257CA1959