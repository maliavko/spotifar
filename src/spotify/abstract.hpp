#ifndef ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#pragma once

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

using httplib::Result;
using utils::far3::synchro_tasks::dispatch_event;

template<class T> class sync_collection;
template<class T> class async_collection;

using followed_artists_t = sync_collection<artist_t>;
using followed_artists_ptr = std::shared_ptr<followed_artists_t>;

using saved_albums_t = async_collection<saved_album_t>;
using saved_albums_ptr = std::shared_ptr<saved_albums_t>;

using saved_tracks_t = async_collection<saved_track_t>;
using saved_tracks_ptr = std::shared_ptr<saved_tracks_t>;

using saved_playlists_t = async_collection<simplified_playlist_t>;
using saved_playlists_ptr = std::shared_ptr<saved_playlists_t>;

using artist_albums_t = async_collection<simplified_album_t>;
using artist_albums_ptr = std::shared_ptr<artist_albums_t>;

using new_releases_t = async_collection<simplified_album_t>;
using new_releases_ptr = std::shared_ptr<new_releases_t>;

using album_tracks_t = async_collection<simplified_track_t>;
using album_tracks_ptr = std::shared_ptr<album_tracks_t>;

struct api_interface
{
    template<class T> friend class item_requester;
    template<class T> friend class sync_collection;
    template<class T> friend class async_collection;

    virtual ~api_interface() {}

    /// @brief A public interface for obtaining a weak pointer to the API interface
    /// instance. Used in many helper classes, avoiding passing a direct pointer for safety reasons
    virtual auto get_ptr() -> std::weak_ptr<api_interface> = 0;

    /// @brief Checks the spotify authorizations status
    virtual bool is_authenticated() const = 0;

    /// @brief Returns a played history list of items. If `force_resync` is true, the data
    /// is forcibly resynced before it is returned
    virtual auto get_play_history(bool force_resync = false) -> const history_items_t& = 0;

    /// @brief Returns a list of available playback devices. If `force_resync` is true, the data
    /// is forcibly resynced before it is returned
    virtual auto get_available_devices(bool force_resync = false) -> const devices_t& = 0;

    /// @brief Returns a currently playing state object. If `force_resync` is true, the data
    /// is forcibly resynced before it is returned
    virtual auto get_playback_state(bool force_resync = false) -> const playback_state_t& = 0;
    
    // library & collections interface

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-top-tracks
    virtual auto get_artist_top_tracks(const item_id_t &artist_id) -> std::vector<track_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-followed
    virtual auto get_followed_artists() -> followed_artists_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artist
    virtual auto get_artist(const item_id_t &artist_id) -> artist_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-artists
    virtual auto get_artists(const item_ids_t &ids) -> std::vector<artist_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-albums
    virtual auto get_artist_albums(const item_id_t &artist_id) -> artist_albums_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-albums
    virtual auto get_saved_albums() -> saved_albums_ptr = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-new-releases
    virtual auto get_new_releases() -> new_releases_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-album
    virtual auto get_album(const item_id_t &album_id) -> album_t = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-albums
    virtual auto get_albums(const item_ids_t &ids) -> std::vector<album_t> = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-albums-tracks
    virtual auto get_album_tracks(const item_id_t &album_id) -> album_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
    virtual auto get_saved_tracks() -> saved_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-playlists-tracks
    virtual auto get_playlist_tracks(const item_id_t &playlist_id) -> saved_tracks_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-list-users-playlists
    virtual auto get_saved_playlists() -> saved_playlists_ptr = 0;

    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-playlist
    virtual auto get_playlist(const item_id_t &playlist_id) -> playlist_t = 0;
    
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-queue
    virtual auto get_playing_queue() -> playing_queue_t = 0;

    virtual auto check_saved_track(const item_id_t &track_id) -> bool = 0;
    virtual auto check_saved_tracks(const item_ids_t &ids) -> std::deque<bool> = 0;
    virtual auto save_tracks(const item_ids_t &ids) -> bool = 0;
    virtual auto remove_saved_tracks(const item_ids_t &ids) -> bool = 0;
    
    /// @brief Returns the downloaded and cached image filepath in case of success or empty string.
    /// @param image the image_t object to fetch from the Spotify server
    /// @param item_id the id of the item the image belongs to (e.g. album id or artist id)
    virtual auto get_image(const image_t &image, const item_id_t &item_id) -> wstring = 0;

    // playback interface
    virtual void start_playback(const string &context_uri, const string &track_uri = "",
        int position_ms = 0, const item_id_t &device_id = "") = 0;
    virtual void start_playback(const std::vector<string> &uris, const item_id_t &device_id = "") = 0;
    virtual void start_playback(const simplified_album_t &album, const simplified_track_t &track) = 0;
    virtual void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track) = 0;
    virtual void resume_playback(const item_id_t &device_id = "") = 0;
    virtual void toggle_playback(const item_id_t &device_id = "") = 0;
    virtual void pause_playback(const item_id_t &device_id = "") = 0;
    virtual void skip_to_next(const item_id_t &device_id = "") = 0;
    virtual void skip_to_previous(const item_id_t &device_id = "") = 0;
    virtual void seek_to_position(int position_ms, const item_id_t &device_id = "") = 0;
    virtual void toggle_shuffle(bool is_on, const item_id_t &device_id = "") = 0;
    virtual void toggle_shuffle_plus(bool is_on) = 0;
    virtual void set_repeat_state(const string &mode, const item_id_t &device_id = "") = 0;
    virtual void set_playback_volume(int volume_percent, const item_id_t &device_id = "") = 0;
    virtual void transfer_playback(const item_id_t &device_id, bool start_playing = false) = 0;
protected:
    /// @brief Performs an HTTP GET request
    /// @param cache_for caches the response for the given amount of time
    virtual Result get(const string &url, utils::clock_t::duration cache_for = {}) = 0;

    /// @brief Performs an HTTP PUT request
    virtual Result put(const string &url, const string &body = {}) = 0;

    /// @brief Performs an HTTP DEL request
    virtual Result del(const string &url, const string &body = {}) = 0;

    /// @brief Performs an HTTP POST request
    virtual Result post(const string &url, const string &body = {}) = 0;

    /// @brief Returns a reference to the internally allocated thread-pool. Used by
    /// requesters to perform async request
    virtual auto get_pool() -> BS::thread_pool& = 0;

    /// @brief Whether the given url is cached
    virtual auto is_request_cached(const string &url) const -> bool = 0;
};

/// @brief A dedicated type used fo passing api interface to all the main plugin's
/// structures as a proxy parameter for the further usage
using api_proxy_ptr = std::weak_ptr<api_interface>;

struct requester_observer: public BaseObserverProtocol
{
    /// @brief The handler is called, when some multi-page requester is about
    /// to execute a remote request
    /// @param url a request's url to identify requests from each other
    virtual void on_request_started(const string &url) {}

    /// @brief The handler is called, when some multi-page requester is about
    /// to finish a remote request execution
    /// @param url a request's url to identify requests from each other
    virtual void on_request_finished(const string &url) {}

    /// @brief Some multi-page requester has received an intermediate result
    /// @param url a requester url
    /// @param progress an amount of data entries accumulated so far
    /// @param total an amount of total entries to receive
    virtual void on_request_progress_changed(const string &url, size_t progress, size_t total) {}
};

/// @brief A helper class to propagate multi-page requesters progress to the listeners
struct [[nodiscard]] requester_progress_notifier
{
    requester_progress_notifier(const string &url): request_url(url)
    {
        ObserverManager::notify(&requester_observer::on_request_started, request_url);
    }

    ~requester_progress_notifier()
    {
        ObserverManager::notify(&requester_observer::on_request_finished, request_url);
    }

    void send_progress(size_t progress, size_t total)
    {
        ObserverManager::notify(&requester_observer::on_request_progress_changed,
            request_url, progress, total);
    }

    string request_url;
};

/// @brief A helper-class for requesting data from spotify api. Incapsulated
/// a logic for performing a request, parsing and holding final result
/// @tparam T a final result's type
template<class T>
class item_requester
{
public:
    using result_t = T;
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
    auto get() const -> const result_t& { return result; }

    auto get_url() const -> const string& { return url; }

    /// @brief Retuns the http-result object
    /// @note result is valid only after a proper request
    auto get_response() const -> const Result& { return response; }

    /// @brief Checks whether the requested result has already been cached and
    /// cab be obtained quickly without a delay
    bool is_cached(api_proxy_ptr api) const
    {
        return !api.expired() && api.lock()->is_request_cached(get_url());
    }

    /// @brief Launches a requester and obtaines a result over http. If `only_cache` is true,
    /// the request will be launched only in case the result is cached and valid to avoid
    /// long waiting delays.
    /// @returns a flag whether the request has been finished without errors
    bool execute(api_proxy_ptr api_proxy, bool only_cached = false)
    {
        if (only_cached && !is_cached(api_proxy))
            return false;

        if (api_proxy.expired()) return false;

        response = api_proxy.lock()->get(url, utils::http::session);
        if (is_success(response))
        {
            try
            {
                // there is no content, no need to parse anything
                if (response->status == httplib::NoContent_204)
                    return true;
                
                auto doc = json::parse(response->body);
                
                json::Value &body = *doc;
                if (!fieldname.empty())
                    body = body[fieldname];

                on_read_result(body, result);
                return true;
            }
            catch (const std::exception &ex)
            {
                log::api->error("There is error while parsing api data response, {}", ex.what());
                return false;
            }
        }
        return false;
    }
protected:
    virtual bool is_success(Result &r) const { return utils::http::is_success(r->status); }

    /// @brief Provides a way for derived classes to specify result parsing approach
    /// @param body parsed response body
    /// @param result a reference to the result to hold
    virtual void on_read_result(const json::Value &body, T &result)
    {
        from_json(body, result);
    }
protected:
    string fieldname;
    string url;
    result_t result;
    Result response;
};

/// @brief A class-helper, used for requesting several items from Spotify. As their
/// API allows requesting with a limited number of items, the requester implements
/// an interface to get data by chunks, stores them in one container and the returns
/// @tparam T a type of the data returned, iterable
/// @tparam ContainerT a type for the result's container-holder
template<class T, class ContainerT = std::vector<typename T>>
class several_items_requester
{
public:
    using result_t = ContainerT;
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
    auto get() const -> const result_t& { return result; }

    bool execute(api_proxy_ptr api, bool only_cached = false)
    {   
        result.clear();

        auto chunk_begin = ids.begin();
        auto chunk_end = ids.begin();
        
        requester_progress_notifier notifier(url);
        
        do
        {
            if (std::distance(chunk_end, ids.end()) <= chunk_size)
                chunk_end = ids.end(); // the number of ids is less than the max chunk size
            else
                std::advance(chunk_end, chunk_size); // ..or just advance iterator further

            notifier.send_progress(result.size(), ids.size());

            item_requester<result_t> requester(url, {
                { "ids", utils::string_join(item_ids_t(chunk_begin, chunk_end), ",") },
            }, data_field);

            if (!requester.execute(api, only_cached))
                return false;
            
            const auto &items = requester.get();
            result.insert(result.end(), items.begin(), items.end());
        
            chunk_begin = chunk_end;
        }
        while (std::distance(chunk_begin, ids.end()) > 0); // there are still items between iterators

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
    auto get_next_url() const -> const string { return next; }

    /// @brief Returns a total amount of items in collection
    /// @note works only a successful request
    size_t get_total() const { return total; }
protected:
    /// @brief A response body parser specialization to get all the necessary fields,
    /// to process further with collection requesting
    void on_read_result(const json::Value &body, T &result) override
    {
        from_json(body["items"], result);

        if (body.HasMember("total") && !body["total"].IsNull())
            total = body["total"].GetUint64();

        if (body.HasMember("next") && !body["next"].IsNull())
            next = body["next"].GetString();
        else
            next = "";
    }
private:
    size_t total = 0;
    string next = "";
};

struct collection_interface
{
    /// @brief Returns the total count of items in the collection, performs a single
    /// server request if needed silently
    virtual auto get_total() const -> size_t = 0;

    /// @brief Returns the total count of items in the collection if available or zero,
    /// works only with cache, does not perform any request
    virtual auto peek_total() const -> size_t = 0;

    /// @brief A public interface method to populate the collection fully
    /// @param only_cached flag, telling the logic, that the method wa called
    /// from some heavy environment and should not perform many http calls
    virtual auto fetch(bool only_cached = false) -> bool = 0;

    /// @brief Returns whether the container is populated from server or not
    virtual bool is_populated() const = 0;
};

using collection_interface_ptr = std::shared_ptr<collection_interface>;

/// @brief A maximum number of one-time requested collection items
static const size_t max_limit = 50ULL;

/// @brief An abstract class of an API collection object. Behaves like a vector,
/// always empty after creation and has to be populated manually. Provides an interface
/// to fetch collection from the server or to get total count of items held in collection
/// without fetching all the data.
template<class T>
class collection_abstract:
    public std::vector<T>,
    public collection_interface
{
public:
    using container_t = std::vector<T>;
    using requester_t = collection_requester<container_t>;
    using requester_ptr = std::shared_ptr<requester_t>;
public:
    /// @param url a url to request
    /// @param params get-request parameters to infuse into given `url`
    /// @param fieldname in case the data has a nested level, this is the field name
    /// to use on the parsed body
    collection_abstract(api_proxy_ptr api, const string &request_url,
                        httplib::Params params = {}, const string &fieldname = ""):
        api_proxy(api), url(request_url), params(params), fieldname(fieldname)
    {
        this->params.insert(std::pair{ "limit", std::to_string(max_limit) });
    }

    size_t get_total() const override
    {
        if (populated) // if the collection is populated, return its size
            return this->size();

        // or perform a first page request
        auto requester = get_begin_requester();
        if (requester->execute(api_proxy))
            return requester->get_total();
        
        return 0LL;
    }

    size_t peek_total() const override
    {
        // returns a total number only if the request is cached
        auto requester = get_begin_requester();
        if (requester->is_cached(api_proxy))
            if (requester->execute(api_proxy))
                return requester->get_total();
        
        return 0LL;
    }

    bool is_populated() const override { return populated; }

    bool fetch(bool only_cached = false)
    {
        this->clear();
        populated = false;

        if (!fetch_all(api_proxy, only_cached))
            return false;

        populated = true;

        return true;
    }
protected:
    /// @brief Returns a first requester to start requesting a collection. Used in some
    /// cases for getting auxilliary info of the collection
    virtual auto get_begin_requester() const -> requester_ptr = 0;

    /// @brief A main requesting logic is implemented here
    /// @param only_cached flag, telling the logic, that the method wa called
    /// from some heavy environment and should not perform many http calls
    virtual auto fetch_all(api_proxy_ptr api, bool only_cached) -> bool = 0;
protected:
    api_proxy_ptr api_proxy;
    string url;
    string fieldname;
    httplib::Params params;
    bool populated = false;
};

/// @brief The items collection, which populates itself, requesting data
/// from the server synchronously page by page
/// @tparam T result data type
template<class T>
class sync_collection: public collection_abstract<T>
{
public:
    using collection_abstract<T>::requester_t;
    using collection_abstract<T>::requester_ptr;
    using collection_abstract<T>::collection_abstract;
protected:
    bool fetch_all(api_proxy_ptr api, bool only_cached) override
    {       
        auto requester = get_begin_requester();
        requester_progress_notifier notifier(requester->get_url());

        while (requester != nullptr)
        {
            // if some of the pages were not requested well, all the operation
            // is aborted
            if (!requester->execute(api, only_cached))
                return false;

            notifier.send_progress(this->size(), requester->get_total());

            // reserving all the container length space at once
            if (this->capacity() != requester->get_total())
                this->reserve(requester->get_total());
            
            auto &items = requester->get();
            this->insert(this->end(), items.begin(), items.end());
    
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

/// @brief The items collection, which populates itself, requesting data
/// from the server asynchronously. Once all the responsed are received,
/// accumulates them in the right order
/// @tparam T result data type
template<class T>
class async_collection: public collection_abstract<T>
{
public:
    using collection_abstract<T>::requester_t;
    using collection_abstract<T>::requester_ptr;
    using collection_abstract<T>::collection_abstract;
protected:
    bool fetch_all(api_proxy_ptr api_proxy, bool only_cached) override
    {   
        if (api_proxy.expired()) return false;

        auto requester = make_requester(0);
        // performing the first request to obtain a total number fo items
        size_t total = 0;
        if (requester->execute(api_proxy, only_cached))
            total = requester->get_total();

        if (total == 0) return false;

        requester_progress_notifier notifier(requester->get_url());

        // calculating the amount of pages
        size_t start = 1ULL, end = total / max_limit;
        if (total - end * max_limit > 0)
            end += 1;

        std::vector<std::vector<T>> result(end);
        result[0] = requester->get();

        /// @note for some reason passing weakref does not work here, it gets `empty`.
        /// So, I am passing real api pointer which works well
        auto api = api_proxy.lock();
        BS::multi_future<void> sequence_future = api->get_pool().submit_sequence(start, end,
            [this, &result, api = api.get(), only_cached, &notifier, total](const size_t idx)
            {
                auto requester = make_requester(idx * max_limit);

                if (!requester->execute(api->get_ptr(), only_cached))
                    throw std::runtime_error("Unsuccesful request");
                
                result[idx] = requester->get();

                size_t items_received = 0;
                for (const auto &chunk: result)
                    items_received += chunk.size();
                
                notifier.send_progress(items_received, total);
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