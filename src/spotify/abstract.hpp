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
    virtual ~api_abstract() {}

    /// @brief Checks the spotify authorizations status
    virtual bool is_authenticated() const = 0;

    virtual bool is_request_cached(const string &url) const = 0;
    virtual void set_frequent_syncs(bool is_on) = 0;
    virtual bool is_frequent_syncs() const = 0;
    
    // library & collections interface
    virtual auto get_available_devices() -> const devices_t& = 0;
    virtual auto get_playback_state() -> const playback_state_t& = 0;
    virtual auto get_followed_artists() -> const artists_t& = 0;
    virtual auto get_artist(const string &artist_id) -> artist_t  = 0;
    virtual auto get_artist_albums(const string &artist_id) -> const simplified_albums_t&  = 0;
    virtual auto get_saved_albums() -> const saved_albums_t& = 0;
    virtual auto get_new_releases() -> const simplified_albums_t& = 0;
    virtual auto get_artist_top_tracks(const string &artist_id) -> tracks_t = 0;
    virtual auto get_album(const string &album_id) -> album_t = 0;
    virtual auto get_album_tracks(const string &album_id) -> const simplified_tracks_t& = 0;
    virtual auto get_playlist(const string &playlist_id) -> playlist_t = 0;
    virtual auto get_playlists() -> const simplified_playlists_t& = 0;
    virtual auto get_playlist_tracks(const string &playlist_id) -> const saved_tracks_t& = 0;
    virtual auto check_saved_track(const string &track_id) -> bool = 0;
    virtual auto check_saved_tracks(const std::vector<string> &ids) -> std::vector<bool> = 0;
    virtual auto save_tracks(const std::vector<string> &ids) -> bool = 0;
    virtual auto remove_saved_tracks(const std::vector<string> &ids) -> bool = 0;
    virtual auto get_playing_queue() -> playing_queue_t = 0;

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

    /// @brief Performs an HTTP GET request
    /// @param cache_for caches the requested data for the given amount of time
    virtual Result get(const string &url, utils::clock_t::duration cache_for = {}) = 0;
    virtual Result put(const string &url, const json &body = {}) = 0;
    virtual Result del(const string &url, const json &body = {}) = 0;
};

/// @brief A class-helper for the API requests, incapsulates some simple logic for 
/// common requests operations. Requests, validates response, parses result,
/// returns mapped data.
///
/// @tparam T a resulting object type, json item
template<class T>
struct api_requester
{
    typedef typename T value_t;
    
    T result; // result holder
    json data; // parsed result json data
    string url; // initial request url string
    string data_field; // some responses have nested data under `data_field` key name
    Result response; // request httplib::Response

    /// @param request_url initial request url
    /// @param params request params object
    /// @param data_field some responses have nested data under `data_field` key name
    api_requester(const string &request_url, httplib::Params params = {}, const string &data_field = ""):
        data_field(data_field)
    {
        url = httplib::append_query_params(request_url, params);
    }
    
    const string &get_url() const { return url; }
    
    /// @brief Returns a reference to the requested data.
    /// @note The result is valid only after a successful response
    const T& get() const { return result; }

    /// @brief Whether the performed response is succeeded
    bool is_success() const { return utils::http::is_success(response->status); }

    /// @brief Executing an API request
    /// @return a flag, whether the request succeeded or not
    bool operator()(api_abstract *api)
    {
        response = api->get(url, utils::http::session);
        if (is_success())
        {
            data = json::parse(response->body);

            // the needed data is nested, we rebind references deeper
            if (!data_field.empty())
                data = data.at(data_field);
            
            on_success(data);

            return true;
        }
        return false;
    }

    /// @brief The method is called right after the valid response is received and
    /// data is parsed correctly. Base method also reads a resulting value in this method
    virtual void on_success(const json &data)
    {
        data.get_to(result);
    }
};

/// @brief A class-helpers to perform the API requests, for the paginated data.
/// @tparam T a resulting data type
template<class T>
struct api_collection_requester: public api_requester<T>
{
    using api_requester<T>::api_requester; // base ctor

    /// @brief Returns the total amount of entries in general
    /// @note The result is valid only after a successful response
    size_t get_total() const { return this->data["total"].get<size_t>(); }

    /// @brief Can be further iterated or not 
    bool has_more() const { return !this->url.empty(); }
    
    virtual void on_success(const json &data)
    {
        data["items"].get_to(this->result);

        auto next = data["next"];
        this->url = !next.is_null() ? next.get<string>() : "";
    }

    /// @brief Iterating items page by page of a given `limit` size
    std::generator<T> fetch_by_pages(api_abstract *api)
    {
        do
        {
            if ((*this)(api))
                co_yield this->result;
        }
        while (this->has_more());
    }
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