#ifndef REQUESTERS_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define REQUESTERS_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#pragma once

#include "stdafx.h"
#include "utils.hpp"
#include "interfaces.hpp"
#include "observer_protocols.hpp"


namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;


/// @brief A maximum number of one-time requested collection items
extern const size_t max_limit;

void http_logger(const httplib::Request &req, const httplib::Response &res);

// a helper function to dispatch a playback command execution error higher
// to the listeners
template <typename... Args>
void playback_cmd_error(string msg_fmt, Args &&...args)
{
    auto formatted = std::vformat(msg_fmt, std::make_format_args(args...));
    
    log::api->error(formatted);

    utils::far3::synchro_tasks::dispatch_event(
        &api_requests_observer::on_playback_command_failed, formatted);
}

/// @brief A helper-formatter to get an error message of a collection fetching requester
template<class R>
static string get_fetching_error(const R &requester)
{
    return std::format("collection fetching error '{}', url '{}'",
        utils::http::get_status_message(requester->get_response()), requester->get_url());
}


/// @brief A helper class to propagate multi-page requesters progress to the listeners
struct [[nodiscard]] requester_progress_notifier
{
    requester_progress_notifier(const string &url, bool is_active = true): request_url(url), is_active(is_active)
    {
        if (is_active)
            ObserverManager::notify(&api_requests_observer::on_request_started, request_url);
    }

    ~requester_progress_notifier()
    {
        if (is_active)
            ObserverManager::notify(&api_requests_observer::on_request_finished, request_url);
    }

    void send_progress(size_t progress, size_t total)
    {
        if (is_active)
            ObserverManager::notify(&api_requests_observer::on_request_progress_changed,
                request_url, progress, total);
    }

    string request_url;
    bool is_active;
};


/// @brief Requester-wrapper for the modifying type of http-request: PUT or DEL
class modify_requester
{
public:
    modify_requester(const string &url, const string &body = ""): url(url), body(body) {}

    auto get_url() const -> const string& { return url; }
    auto get_body() const -> const string& { return body; }
    auto get_response() const -> const httplib::Result& { return response; }

    bool execute(api_weak_ptr_t api_proxy);
protected:
    /// @brief Should implement a real request, depending on the derived type class
    /// specification 
    virtual httplib::Result execute_request(api_interface *api) = 0;
private:
    string url;
    string body;
    httplib::Result response;
};

/// @brief A PUT http-request requester specialization
class put_requester: public modify_requester
{
public:
    using modify_requester::modify_requester;
protected:
    httplib::Result execute_request(api_interface *api) override;
};

/// @brief A DEL http-request requester specialization
class del_requester: public modify_requester
{
public:
    using modify_requester::modify_requester;
protected:
    httplib::Result execute_request(api_interface *api) override;
};


/// @brief Search requester
/// https://developer.spotify.com/documentation/web-api/reference/search
class search_requester
{
public:
    struct filters_t
    {
        std::vector<string> types;
        string album = "";
        string artist = "";
        string track = "";
        string year = "";
        string genre = "";
        string upc = "";
        string isrc = "";
        bool is_fresh = false;
        bool is_low = false;
    };
public:
    search_requester(const string &search, const filters_t &filters)
    {
        std::vector<string> query{ search };

        if (auto f = utils::trim(filters.album); !f.empty())
            query.push_back(std::format("album={}", f));

        if (auto f = utils::trim(filters.artist); !f.empty())
            query.push_back(std::format("artist={}", f));

        if (auto f = utils::trim(filters.track); !f.empty())
            query.push_back(std::format("track={}", f));

        if (auto f = utils::trim(filters.year); !f.empty())
            query.push_back(std::format("year={}", f));

        if (auto f = utils::trim(filters.genre); !f.empty())
            query.push_back(std::format("genre={}", f));

        if (auto f = utils::trim(filters.upc); !f.empty())
            query.push_back(std::format("upc={}", f));

        if (auto f = utils::trim(filters.isrc); !f.empty())
            query.push_back(std::format("isrc={}", f));

        if (filters.is_fresh)
            query.push_back(std::format("tag:new"));

        if (filters.is_low)
            query.push_back(std::format("tag:hipster"));

        httplib::Params params{
            { "q", utils::string_join(query, ",") },
            { "type", utils::string_join(filters.types, ",") },
            { "limit", "15" },
        };

        url = httplib::append_query_params("/v1/search", params);
    }

    bool execute(api_weak_ptr_t api_proxy)
    {
        if (api_proxy.expired()) return false;

        requester_progress_notifier notifier(url);

        auto response = api_proxy.lock()->get(url);
        if (!utils::http::is_success(response))
        {
            log::api->error("There is an error while executing API fetching request: '{}', "
                "url '{}'", utils::http::get_status_message(response), url);
            return false;
        }
        
        try
        {
            // there is no content, no need to parse anything
            if (response->status == httplib::NoContent_204)
                return true;
            
            json::Document doc;
            doc.Parse(response->body);
            
            if (doc.HasMember("tracks"))
                from_json(doc["tracks"]["items"], tracks);
            
            if (doc.HasMember("artists"))
                from_json(doc["artists"]["items"], artists);

            if (doc.HasMember("albums"))
                from_json(doc["albums"]["items"], albums);

            if (doc.HasMember("playlists"))
                from_json(doc["playlists"]["items"], playlists);

            return true;
        }
        catch (const std::exception &ex)
        {
            log::api->error("There is an error while parsing api data response: {}. "
                "Request '{}', data '{}'", ex.what(), url, response->body);
            return false;
        }
    }
    
    std::vector<spotify::track_t> tracks;
    std::vector<spotify::artist_t> artists;
    std::vector<spotify::simplified_album_t> albums;
    std::vector<spotify::simplified_playlist_t> playlists;
    string url;
};


/// @brief A helper-class for requesting data from spotify api. Incapsulates
/// a logic for performing a request, parsing and holding final result
/// @tparam T a final result's type
/// @tparam N a number of days/hours/mins etc. the request's result will be cached for
/// @tparam C a caching class type: std::chrono::seconds, *::milliseconds, *::weeks etc.
template<class T, int N, class C>
class item_requester
{
public:
    using result_t = T;
public:
    /// @param url a url to request
    /// @param params get-request parameters to infuse into given `url`
    /// @param fieldname in case the data has a nested level, this is the field name
    /// to use on the parsed body
    item_requester(const string &url, httplib::Params params = {}, const string &fieldname = ""):
        url(httplib::append_query_params(url, params)),
        fieldname(fieldname)
        {}
    
    /// @brief Returns a result. Valid only after a successful request
    auto get() const -> const result_t& { return result; }

    auto get_url() const -> const string& { return url; }

    /// @brief Returns the http-result object
    /// @note result is valid only after a proper request
    auto get_response() const -> const httplib::Result& { return response; }

    /// @brief Tells, whether the response is different from the cached one
    bool is_modified() const { return response && response->status != httplib::NotModified_304; }

    /// @brief Checks whether the requested result has already been cached and
    /// cab be obtained quickly without a delay
    bool is_cached(api_weak_ptr_t api) const
    {
        return !api.expired() && api.lock()->is_request_cached(get_url());
    }

    /// @brief Launches a requester and retrieves a result over http. If `only_cache` is true,
    /// the request will be launched only in case the result is cached and valid to avoid
    /// long waiting delays.
    /// @param only_cached returns a valid result only if it was cached previously and
    /// the cache is still valid
    /// @returns `true` in case of: no errors, 204 No Content response, `only_cached` is true, but
    /// no cache exists
    bool execute(api_weak_ptr_t api_proxy, bool only_cached = false, bool retry_429 = false)
    {
        if (only_cached && !is_cached(api_proxy))
            return true;

        if (api_proxy.expired()) return false;

        response = api_proxy.lock()->get(url, C{ N }, retry_429);
        if (!is_success(response))
        {
            log::api->error("There is an error while executing API fetching request: '{}', "
                "url '{}'", utils::http::get_status_message(response), url);
            return false;
        }
        
        try
        {
            // there is no content, no need to parse anything
            if (response->status == httplib::NoContent_204)
                return true;
            
            json::Document doc;
            doc.Parse(response->body);
            
            json::Value &body = doc;
            if (!fieldname.empty())
                body = body[fieldname];

            on_read_result(body, result);
            return true;
        }
        catch (const std::exception &ex)
        {
            log::api->error("There is an error while parsing api data response: {}. "
                "Request '{}', data '{}'", ex.what(), url, response->body);
            return false;
        }
    }
protected:
    virtual bool is_success(const httplib::Result &r) const { return utils::http::is_success(r); }

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
    httplib::Result response;
};


/// @brief A class-helper, used for requesting several items from Spotify. As their
/// API allows requesting with a limited number of items, the requester implements
/// an interface to get data by chunks, stores them in one container and the returns
/// @tparam T a type of the data returned, iterable
/// @tparam N a number of days/hours/mins etc. the request's result will be cached for
/// @tparam C a caching class type: std::chrono::seconds, *::milliseconds, *::weeks etc.
/// @tparam ContainerT a type for the result's container-holder
template<class T, int N, class C, class ContainerT>
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

    /// @brief see `item_requester::execute` interface
    bool execute(api_weak_ptr_t api, bool only_cached = false, bool notify_watchers = true)
    {   
        result.clear();

        auto chunk_begin = ids.begin();
        auto chunk_end = ids.begin();
        
        requester_progress_notifier notifier(url, notify_watchers);
        
        do
        {
            if (std::distance(chunk_end, ids.end()) <= chunk_size)
                chunk_end = ids.end(); // the number of ids is less than the max chunk size
            else
                std::advance(chunk_end, chunk_size); // ..or just advance iterator further

            notifier.send_progress(result.size(), ids.size());

            item_requester<result_t, N, C> requester(url, {
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
/// @tparam N a number of days/hours/mins etc. the request's result will be cached for
/// @tparam C a caching class type: std::chrono::seconds, *::milliseconds, *::weeks etc.
template<class T, int N, class C>
class collection_requester: public item_requester<T, N, C>
{
public:
    using item_requester<T, N, C>::item_requester;

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
            total = body["total"].GetUint();

        if (body.HasMember("next") && !body["next"].IsNull())
            next = body["next"].GetString();
        else
            next = "";
    }
private:
    size_t total = 0;
    string next = "";
};


/// @brief An abstract class of an API collection object. Behaves like a vector,
/// always empty after creation and has to be populated manually. Provides an interface
/// to fetch collection from the server or to get total count of items held in collection
/// without fetching all the data.
/// @tparam T a final result's type
/// @tparam N a number of days/hours/mins etc. the request's result will be cached for
/// @tparam C a caching class type: std::chrono::seconds, *::milliseconds, *::weeks etc.
template<class T, int N, class C>
class collection_abstract:
    public std::vector<T>,
    public collection_interface
{
public:
    using base_t = std::vector<T>;
    using container_t = std::vector<T>;
    using requester_t = collection_requester<container_t, N, C>;
    using requester_ptr = std::shared_ptr<requester_t>;
public:
    /// @param url a url to request
    /// @param params get-request parameters to infuse into given `url`
    /// @param fieldname in case the data has a nested level, this is the field name
    /// to use on the parsed body
    collection_abstract(api_weak_ptr_t api, const string &request_url,
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

    bool is_cached() const override
    {
        // returns whether the first requester's results is cached or not, for most of the
        // cases that means that all of the requesting sequence chain is cached too
        return get_begin_requester()->is_cached(api_proxy);
    }

    bool is_populated() const override { return populated; }

    void clear()
    {
        base_t::clear();
        populated = false;
    }

    /// @param only_cached flag, telling the logic, that the method wa called
    /// from some heavy environment and should not perform many http calls
    /// @param notify_watchers does not send changes to the requesting status observers like
    /// showing request progress splashing screen and etc.
    /// @param pages_to_request number of data pages to request; "0" means all
    bool fetch(bool only_cached = false, bool notify_watchers = true, size_t pages_to_request = 0)
    {
        clear();

        if (!fetch_items(api_proxy, only_cached, notify_watchers, pages_to_request))
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
    /// @param notify_watchers does not send changes to the requesting status observers like
    /// showing request progress splashing screen and etc.
    /// @param pages_to_request number of data pages to request; "0" means all
    virtual bool fetch_items(api_weak_ptr_t api, bool only_cached, bool notify_watchers = true,
        size_t pages_to_request = 0) = 0;
protected:
    api_weak_ptr_t api_proxy;
    string url;
    string fieldname;
    httplib::Params params;
    bool populated = false;
};


/// @brief The items collection, which populates itself, requesting data
/// from the server synchronously page by page
/// @tparam T a final result's type
/// @tparam N a number of days/hours/mins etc. the request's result will be cached for
/// @tparam C a caching class type: std::chrono::seconds, *::milliseconds, *::weeks etc.
template<class T, int N, class C>
class sync_collection: public collection_abstract<T, N, C>
{
public:
    using base_t = collection_abstract<T, N, C>;
    using base_t::requester_t;
    using base_t::requester_ptr;
    using base_t::collection_abstract;
    
    bool is_modified() const override
    {
        return modified;
    }
protected:
    bool fetch_items(api_weak_ptr_t api, bool only_cached, bool notify_watchers = true,
        size_t pages_to_request = 0) override
    {       
        auto requester = get_begin_requester();
        requester_progress_notifier notifier(requester->get_url(), notify_watchers);

        while (requester != nullptr)
        {
            // if some of the pages were not requested well, all the operation is aborted
            if (!requester->execute(api, only_cached, true))
            {
                dispatch_event(&api_requests_observer::on_collection_fetching_failed,
                    get_fetching_error(requester));
                return false;
            }

            if (requester->is_modified())
                modified = true;
            
            auto total = requester->get_total();
            if (pages_to_request > 0)
                total = std::min(total, pages_to_request * max_limit);

            notifier.send_progress(this->size(), total);

            // reserving all the container length space at once
            if (this->capacity() != total)
                this->reserve(total);
            
            const auto &items = requester->get();
            this->insert(this->end(), items.begin(), items.end());
            
            if (--pages_to_request <= 0) break;
    
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
private:
    bool modified = false;
};


/// @brief The items collection, which populates itself, requesting data
/// from the server asynchronously. Once all the responseы are received,
/// accumulates them in the right order.
/// @tparam T a final result's type
/// @tparam N a number of days/hours/mins etc. the request's result will be cached for
/// @tparam C a caching class type: std::chrono::seconds, *::milliseconds, *::weeks etc.
template<class T, int N, class C>
class async_collection: public collection_abstract<T, N, C>
{
public:
    using base_t = collection_abstract<T, N, C>;
    using base_t::requester_t;
    using base_t::requester_ptr;
    using base_t::collection_abstract;
    
    bool is_modified() const override
    {
        return modified;
    }
protected:
    bool fetch_items(api_weak_ptr_t api_proxy, bool only_cached, bool notify_watchers = true,
        size_t pages_to_request = 0) override
    {   
        if (api_proxy.expired()) return false;

        auto requester = make_requester(0);
        // performing the first request to obtain a total number fo items
        if (!requester->execute(api_proxy, only_cached, true))
        {
            dispatch_event(&api_requests_observer::on_collection_fetching_failed,
                get_fetching_error(requester));
            return false;
        }
        
        if (requester->is_modified())
            modified = true;

        size_t total = requester->get_total();
        if (total == 0) // if there is no entries, the results is still valid
            return true;

        requester_progress_notifier notifier(requester->get_url(), notify_watchers);

        // calculating the amount of pages
        size_t start = 1ULL, end = total / max_limit;
        if (total - end * max_limit > 0) // if there are some entries left, adding extra page on top
            end += 1;

        if (pages_to_request > 0)
        {
            end = std::min(pages_to_request, end);
            total = std::min(total, pages_to_request * max_limit);
        }

        std::vector<std::vector<T>> result(end);
        result[0] = requester->get();

        /// @note for some reason passing weakref does not work here, it gets `empty`.
        /// So, I am passing real api pointer which works well
        auto api = api_proxy.lock();
        auto sequence_future = api->get_pool().submit_sequence(start, end,
            [this, &result, api = api.get(), &notifier, total, only_cached, notify_watchers]
            (const size_t idx)
            {
                auto requester = make_requester(idx * max_limit);

                // all the exceptions are being accumulated and rethrown by thread-pool
                // library later
                if (!requester->execute(api->get_ptr(), only_cached, true))
                    throw std::runtime_error(get_fetching_error(requester));
                
                if (requester->get_response()->status != httplib::NotModified_304)
                    modified = true;
                
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
        catch (const string &message)
        {
            dispatch_event(&api_requests_observer::on_collection_fetching_failed, message);
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

    // a collection's requester fabric
    requester_ptr make_requester(size_t offset) const
    {
        auto updated_params = this->params;
        updated_params.insert(std::pair{ "offset", std::to_string(offset) });

        return requester_ptr(new requester_t(this->url, updated_params, this->fieldname));
    }
private:
    bool modified = false;
};

} // namespace spotify
} // namespace spotifar

#endif // REQUESTERS_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6