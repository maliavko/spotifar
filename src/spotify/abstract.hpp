#ifndef ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#pragma once

#include "utils.hpp"
#include "config.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

using httplib::Result;

template<class T> class sync_collection;
template<class T> class async_collection;

typedef async_collection<saved_album_t> saved_albums_t;
typedef std::shared_ptr<saved_albums_t> saved_albums_ptr;

typedef sync_collection<artist_t> followed_artists_t;
typedef std::shared_ptr<followed_artists_t> followed_artists_ptr;

typedef async_collection<simplified_album_t> artist_albums_t;
typedef std::shared_ptr<artist_albums_t> artist_albums_ptr;

typedef async_collection<simplified_album_t> new_releases_t;
typedef std::shared_ptr<new_releases_t> new_releases_ptr;

typedef async_collection<simplified_track_t> album_tracks_t;
typedef std::shared_ptr<album_tracks_t> album_tracks_ptr;

typedef sync_collection<history_item_t> recently_played_tracks_t;
typedef std::shared_ptr<recently_played_tracks_t> recently_played_tracks_ptr;

struct api_abstract
{
    template<class T> friend class sync_collection;
    template<class T> friend class async_collection;

    virtual ~api_abstract() {}

    /// @brief Checks the spotify authorizations status
    virtual bool is_authenticated() const = 0;
    
    // library & collections interface
    virtual auto get_play_history() -> const history_items_t& = 0;
    virtual auto get_available_devices() -> const devices_t& = 0;
    virtual auto get_playback_state() -> const playback_state_t& = 0;

    virtual auto get_artist_top_tracks(const string &artist_id) -> tracks_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-followed
    virtual auto get_followed_artists() -> followed_artists_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artist
    virtual auto get_artist(const string &artist_id) -> artist_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-artists
    virtual auto get_artists(const item_ids_t &ids) -> std::vector<artist_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-albums
    virtual auto get_artist_albums(const string &artist_id) -> artist_albums_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-albums
    virtual auto get_saved_albums() -> saved_albums_ptr = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-new-releases
    virtual auto get_new_releases() -> new_releases_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-album
    virtual auto get_album(const string &album_id) -> album_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-albums
    virtual auto get_albums(const item_ids_t &ids) -> std::vector<album_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-albums-tracks
    virtual auto get_album_tracks(const string &album_id) -> album_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-recently-played
    virtual auto get_recently_played(std::int64_t after) -> recently_played_tracks_ptr = 0;

    virtual auto get_playlist(const string &playlist_id) -> playlist_t = 0;
    virtual auto get_playlists() -> const simplified_playlists_t& = 0;
    virtual auto get_playlist_tracks(const string &playlist_id) -> const saved_tracks_t& = 0;
    virtual auto check_saved_track(const string &track_id) -> bool = 0;
    virtual auto check_saved_tracks(const item_ids_t &ids) -> std::vector<bool> = 0;
    virtual auto save_tracks(const item_ids_t &ids) -> bool = 0;
    virtual auto remove_saved_tracks(const item_ids_t &ids) -> bool = 0;
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
    //TODO: return here protected specifier
    /// @brief Performs an HTTP GET request
    /// @param cache_for caches the requested data for the given amount of time
    virtual Result get(const string &url, utils::clock_t::duration cache_for = {}) = 0;
    virtual Result put(const string &url, const json &body = {}) = 0;
    virtual Result del(const string &url, const json &body = {}) = 0;
    virtual auto is_request_cached(const string &url) const -> bool = 0;
protected:
    virtual auto get_pool() -> BS::thread_pool& = 0;
};

/// @brief A maximum number of one-time requested collection items
static const size_t max_limit = 50ULL;

template<typename>
struct is_std_vector : std::false_type {};

template<typename T, typename A>
struct is_std_vector<std::vector<T,A>> : std::true_type {};

/// @brief A helper-class for requesting data from spotify api. Incapsulated
/// a logic for performing a request, parsing and holding final result
/// @tparam T a final result's type
template<class T>
class item_requester
{
public:
    typedef T result_t;
public:
    /// @param url a url to request
    /// @param params get-request parameters to infuse into given `url`
    /// @param fieldname in case the data has a nested level, this is the field name
    /// to use on the parsed body
    item_requester(const string &url, httplib::Params params = {},
                   const string &fieldname = ""):
        url(httplib::append_query_params(url, params)),
        fieldname(fieldname)
        {}
    
    /// @brief Returns a result. Valid only after a successful request
    const result_t& get() const { return result; }

    const string& get_url() const { return url; }

    bool is_cached(api_abstract *api) const
    {
        return api->is_request_cached(get_url());
    }

    bool execute(api_abstract *api, bool only_cached = false)
    {
        if (only_cached && !is_cached(api))
            return false;
        
        auto res = api->get(url, utils::http::session);
        if (utils::http::is_success(res->status))
        {
            try
            {
                rapidjson::Document document;
                rapidjson::Value &body = document.Parse(res->body);
                
                if (!fieldname.empty())
                    body = body[fieldname];

                on_read_result(body, result);
            }
            catch (const std::exception &ex)
            {
                utils::log::api->error("There is error while parsing api data response, {}",
                    ex.what());
                return false;
            }
            return true;
        }
        return false;
    }
protected:
    /// @brief Provides a way for derived classes to specify result parsing approach
    /// @param body parsed response body
    /// @param result a reference to the result to hold
    virtual void on_read_result(const rapidjson::Value &body, T &result)
    {
        from_rapidjson(body, result);
    }
protected:
    string fieldname;
    string url;
    result_t result;
};

/// @brief A class-helpers to request several items from Spotify. As their
/// API allows requesting with a limited number of items, the requester implements
/// an interface to get data by chunks.
/// @tparam T the tyope of the data returned, iterable
template<class T>
class several_items_requester
{
public:
    typedef std::vector<typename T> result_t;
public:
    /// @param chunk_size a max size of a data chunk to request
    /// @param data_field some responses have nested data under `data_field` key name
    several_items_requester(
        const string &url, const item_ids_t ids, size_t chunk_size,
        const string &data_field = ""
    ):
        url(url), chunk_size(chunk_size), ids(ids), data_field(data_field)
        {}
    
    /// @brief Returns a result. Valid only after a successful request
    const result_t& get() const { return result; }

    bool execute(api_abstract *api, bool only_cached = false)
    {
        result.clear();

        auto chunk_begin = ids.begin();
        auto chunk_end = ids.begin();

        do
        {
            if (std::distance(chunk_end, ids.end()) <= chunk_size)
                chunk_end = ids.end(); // the number of ids is less than the max chunk size
            else
                std::advance(chunk_end, chunk_size); // ..or just advance iterator further

            item_requester<result_t> requester(url, {
                { "ids", utils::string_join(item_ids_t(chunk_begin, chunk_end), ",") },
            }, data_field);
            
            if (!requester.execute(api, only_cached))
                return false;
            
            const auto &items = requester.get();
            result.insert(result.end(), items.begin(), items.end());
        
            chunk_begin = chunk_end;
        }
        while (std::distance(chunk_begin, ids.end()) > 0);

        return true;
    }
private:
    result_t result;
    ptrdiff_t chunk_size;
    item_ids_t ids;
    string data_field;
    string url;
};

/// @brief A requester specialization for getting collections page by page
/// @tparam T a final result's type
template<class T>
class collection_requester: public item_requester<T>
{
public:
    using item_requester<T>::item_requester;

    /// @brief Return a valid `url` to the next page in case it exists
    /// @note works only a successful request
    const string get_next_url() const { return !next.is_null() ? next.get<string>() : ""; }

    /// @brief Returns a total amount of items in collection
    /// @note works only a successful request
    size_t get_total() const { return total; }
protected:
    /// @brief A response body parser specialization to get all the necessary fields,
    /// to process further with collection requesting
    void on_read_result(const rapidjson::Value &body, T &result) override
    {
        from_rapidjson(body["items"], result);

        if (body.HasMember("total") && !body["total"].IsNull())
            total = body["total"].GetUint64();

        if (body.HasMember("next") && !body["next"].IsNull())
            next = body["next"].GetString();
        else
            next = nullptr;
    }
private:
    size_t total = 0;
    json next = nullptr;
};

/// @brief An abstract class of an API collection object. Provides an interface
/// to fetch collection from server, to get total count of items in collection
/// without fetching all the data.
template<class T>
class collection_abstract: public std::vector<T>
{
public:
    typedef std::vector<T> container_t;
    typedef collection_requester<container_t> requester_t;
    typedef std::shared_ptr<requester_t> requester_ptr;
public:
    /// @param url a url to request
    /// @param params get-request parameters to infuse into given `url`
    /// @param fieldname in case the data has a nested level, this is the field name
    /// to use on the parsed body
    collection_abstract(api_abstract *api, const string &request_url,
                        httplib::Params params = {}, const string &fieldname = ""):
        api_proxy(api), url(request_url), params(params), fieldname(fieldname)
    {
        this->params.insert(std::pair{ "limit", std::to_string(max_limit) });
    }

    /// @brief Returns the total count of items in the collection, performs a single
    /// server request if needed silently
    size_t get_total() const
    {
        if (is_populated) // if the collection is populated, return its size
            return this->size();

        // or perform a first page request
        auto requester = get_begin_requester();
        if (requester->execute(api_proxy))
            return requester->get_total();
        
        return 0LL;
    }

    /// @brief Returns the total count of items in the collection if available or zero,
    /// works only with cache, does not perform any request
    size_t peek_total() const
    {
        // returns a total number only if the request is cached
        auto requester = get_begin_requester();
        if (requester->is_cached(api_proxy))
            if (requester->execute(api_proxy))
                return requester->get_total();
        
        return 0LL;
    }

    bool fetch(bool only_cached = false)
    {
        this->clear();

        if (!fetch_all(api_proxy, only_cached))
            return false;

        is_populated = true;

        return true;
    }
protected:
    virtual auto get_begin_requester() const -> requester_ptr = 0;

    virtual auto fetch_all(api_abstract *api, bool only_cached) -> bool = 0;
protected:
    api_abstract *api_proxy;
    string url;
    string fieldname;
    httplib::Params params;
    bool is_populated = false;
};

/// @brief Performs populating the collection page by page, each page
/// has a link to the next one
/// @tparam T result data type
template<class T>
class sync_collection: public collection_abstract<T>
{
public:
    using collection_abstract<T>::requester_t;
    using collection_abstract<T>::requester_ptr;
    using collection_abstract<T>::collection_abstract;
protected:
    bool fetch_all(api_abstract *api, bool only_cached) override
    {
        auto requester = get_begin_requester();

        while (requester != nullptr)
        {
            if (!requester->execute(api, only_cached))
                return false;

            if (this->capacity() != requester->get_total())
                this->reserve(requester->get_total());
                
            auto &entries = requester->get();
            this->insert(this->end(), entries.begin(), entries.end());
    
            const auto &next_url = requester->get_next_url();
            if (!next_url.empty())
                requester = requester_ptr(new requester_t(next_url, {}, this->fieldname));
            else 
                requester = nullptr;
        }

        return true;
    }

    requester_ptr get_begin_requester() const override
    {
        return requester_ptr(new requester_t(this->url, this->params, this->fieldname));
    }
};

/// @brief Performs populating the collection asynchronously
/// @tparam T result data type
template<class T>
class async_collection: public collection_abstract<T>
{
public:
    using collection_abstract<T>::requester_t;
    using collection_abstract<T>::requester_ptr;
    using collection_abstract<T>::collection_abstract;
protected:
    bool fetch_all(api_abstract *api, bool only_cached) override
    {
        // performing the first request to obtain a total number fo items
        auto requester = make_requester(0);

        size_t total = 0;
        if (requester->execute(api, only_cached))
            total = requester->get_total();

        if (total == 0) return false;

        // calculating an amount of pages
        size_t start = 1ULL, end = total / max_limit;
        if (total - end * max_limit > 0)
            end += 1;

        std::vector<std::vector<T>> result(end);
        result[0] = requester->get();

        BS::multi_future<void> sequence_future = api->get_pool().submit_sequence(start, end,
            [this, &result, api, only_cached](const size_t idx)
            {
                auto requester = make_requester(idx * max_limit);

                if (!requester->execute(api, only_cached))
                    throw std::runtime_error("Unsuccesful request");
                
                result[idx] = requester->get();
            });

        try
        {
            sequence_future.wait();
            sequence_future.get();
        }
        catch (const std::runtime_error&)
        {
            return false;
        }

        // preallocating data container for holding the final result
        if (this->capacity() != total)
            this->reserve(total);

        for (const auto &chunk: result)
            this->insert(this->end(), chunk.begin(), chunk.end());

        return true;
    }

    requester_ptr get_begin_requester() const override
    {
        return make_requester(0);
    }

    requester_ptr make_requester(size_t offset) const
    {
        auto updated_params = this->params;
        updated_params.insert(std::pair{ "offset", std::to_string(offset) });

        return requester_ptr(new requester_t(this->url, updated_params, this->fieldname));
    }
};

} // namespace spotify
} // namespace spotifar

#endif // ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6