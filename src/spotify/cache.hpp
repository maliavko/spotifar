#ifndef CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5
#define CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5
#pragma once

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;
using config::persistent_data;
using settings_ctx = config::settings_context;

/// @brief An interface to the class, which implements the functionality to cache the data
/// and store it in the local storage
struct cached_data_abstract: public config::persistent_data_abstract
{
    virtual void shutdown(config::settings_context &ctx) { write(ctx); }
    
    /// @brief An method to resync data from the server
    /// @param force - if true, the data will be resynced regardless of the cache validity
    virtual void resync(bool force = false) = 0;

    /// @brief Return true if the cache should not be resynced
    virtual bool is_active() const { return true; }
};

/// @brief A class to store a json value in the persistent storage
/// @tparam T - an item type, which implements json serialization
template<class T>
class json_value: public persistent_data<T>
{
public:
    using persistent_data<T>::persistent_data; // default ctor

protected:
    virtual void read_from_storage(settings_ctx &ctx, const wstring &key, T &data)
    {
        auto storage_value = ctx.get_str(key, "");
        if (!storage_value.empty())
            json::parse_to(storage_value, data);
    }

    virtual void write_to_storage(settings_ctx &ctx, const wstring &key, const T &data)
    {
        ctx.set_str(key, json::dump(data)->GetString());
    }
};


using clock_t = utils::clock_t;
using time_point = clock_t::time_point;

/// @brief A class to hold a timestamp and store it in the far settings storage
class timestamp_value: public persistent_data<time_point>
{
public:
    using persistent_data<time_point>::persistent_data; // default ctor

protected:
    virtual void read_from_storage(settings_ctx &ctx, const wstring &key, time_point &data)
    {
        auto storage_timestamp = ctx.get_int64(key, 0LL);
        data = time_point{ clock_t::duration(storage_timestamp) };
    }

    virtual void write_to_storage(settings_ctx &ctx, const wstring &key, const time_point &data)
    {
        ctx.set_int64(key, data.time_since_epoch().count());
    }
};

/// @brief Class implements a functionality to sync, cache and store a json data.
/// The interface provides a way to specify the data request method, caching interval,
/// and storage details.
/// @tparam T - type of a cached json item
template<class T>
class json_cache: public cached_data_abstract
{
public:
    using patch_handler_t = std::function<void(json::Document &)>;
public:
    /// @param storage_key A storage key name to save the data to. If key is empty, so
    /// the cache is not getting saved to disk, kept for sessino only
    json_cache(const wstring &storage_key = L"");

    // persistent data interface
    void read(settings_ctx &ctx);
    void write(settings_ctx &ctx);
    void clear(settings_ctx &ctx);

    // cached data interface
    void resync(bool force = false);

    /// @brief Spotify does not send back an immediate updated data after successfully
    /// performed command, so we need to apply patches to the data to keep it up-to-date
    /// for some time for the all upcoming resyncs
    void patch(patch_handler_t handler);

    auto get() const -> const T& { return data.get(); }
    auto get_last_sync_time() const -> const time_point& { return last_sync_time.get(); }
    auto get_expires_at() const -> const time_point { return get_last_sync_time() + get_sync_interval(); }
    bool is_valid() const { return get_expires_at() > clock_t::now(); }
    void invalidate() { last_sync_time.set({}); }
protected:
    /// @brief The class calls the method when the data should be resynced. An implementation
    /// should return true if the data was successfully resynced, and put the data to the 
    /// given reference `json_data`
    /// @param json_data 
    virtual bool request_data(T &json_data) = 0;

    /// @brief The amount of time the data is valid and will not be resynced
    virtual auto get_sync_interval() const -> clock_t::duration = 0;
    
    void apply_patches(T &item);

    /// @brief The method is called when the data is successfully resynced, either
    /// from the server or from the cache. Ideal place for post-processing
    /// @param data a currently synced data
    /// @param prev_data a previously synced data to compare with
    virtual void on_data_synced(const T &data, const T &prev_data) {}
private:
    json_value<T> data;
    timestamp_value last_sync_time;
    bool is_persistent;

    std::mutex patch_mutex;
    std::vector<std::pair<time_point, patch_handler_t>> patches;
};

template<typename T>
json_cache<T>::json_cache(const wstring &storage_key):
    is_persistent(!storage_key.empty()),
    data(storage_key),
    last_sync_time(storage_key + L"Time")
{
}

template<typename T>
void json_cache<T>::read(settings_ctx &ctx)
{
    if (is_persistent)
    {
        data.read(ctx);
        last_sync_time.read(ctx);
    }

    // if the data is still valid, we send notification as
    // it was resynced well from server
    if (is_valid())
        on_data_synced(data.get(), data.get());
}

template<typename T>
void json_cache<T>::write(settings_ctx &ctx)
{
    if (is_persistent)
    {
        data.write(ctx);
        last_sync_time.write(ctx);
    }
}

template<typename T>
void json_cache<T>::clear(settings_ctx &ctx)
{
    if (is_persistent)
    {
        data.clear(ctx);
        last_sync_time.clear(ctx);
    }
}

template<typename T>
void json_cache<T>::resync(bool force)
{
    auto sync_time = clock_t::now();
    // no updates for disabled caches, otherwise only in case the data
    // is invalid or resync is forced
    if (!force && (!is_active() || is_valid()))
        return;

    T new_data;
    if (request_data(new_data))
    {
        auto old_data = data.get();
        apply_patches(new_data);

        data.set(new_data);
        last_sync_time.set(sync_time);

        on_data_synced(new_data, old_data);
    }
}

template<typename T>
void json_cache<T>::patch(json_cache<T>::patch_handler_t handler)
{
    // patches are saved and applied next time data is resynced
    std::lock_guard lock(patch_mutex);
    patches.push_back(std::make_pair(clock_t::now(), handler));

    // instead of calling "resync", we invalidate the cache, so it is updated in
    // a correct order from the right thread
    invalidate();
}

template<typename T>
void json_cache<T>::apply_patches(T &item)
{
    if (patches.empty()) return;

    // unpacking and packing the item here, definitely not the best solution,
    // did not come up with the better one still
    // json j = item; 
    auto now = clock_t::now();

    json::Document doc;
    to_json(doc, item, doc.GetAllocator());

    // removing outdated patches first
    std::erase_if(patches, [&now](auto &v) { return v.first + 1500ms < now; });
    
    // applying the valid patches next
    for (const auto& [t, p]: patches)
        p(doc);
    
    // unpacking the item back to the data
    from_json(doc, item);
}

/// @brief A class-helper for caching http responses from spotify server. Holds
/// the information about ETags and validity time of the responses
class http_cache
{
public:
    /// @brief A calss for storing one entry of a cache
    struct cache_entry
    {
        string etag = "";
        string body = "";
        clock_t::time_point cached_until{};
        
        /// @brief Is cached value still valid or expired
        inline bool is_valid() const { return clock_t::now() < cached_until; }

        /// @brief Is value cached only for the current session or persistent
        inline bool is_cached_for_session() const { return cached_until == clock_t::time_point::max(); }

        // json serialization interface
        friend void from_json(const json::Value &j, cache_entry &e);
        friend void to_json(json::Value &j, const cache_entry &e, json::Allocator allocator);
    };
public:
    void start();
    void shutdown();

    /// @brief Initialization can take some time, return readiness
    inline bool is_initilized() const { return is_initialized; }

    /// @brief Store given reponse body for a cache
    /// @param url http get-request
    /// @param body a body to store
    /// @param http response ETag header
    /// @param cache_for optional time duration to keep the response
    void store(const string &url, string body, const string &etag, clock_t::duration cache_for = {});
    void store(const string &url, const cache_entry &entry);

    /// @brief Does cache have the stored response for a given `url`
    bool is_cached(const string &url) const;

    /// @brief Get the cached data for a given `url`
    auto get(const string &url) const -> const cache_entry&;

    /// @brief Invalidates stored data for the `url` or range of urls, matching given `url part`
    void invalidate(const string &url_part);

    /// @brief Invalidates all the cache
    void clear_all();
private:
    mutable std::mutex guard;
    std::atomic<bool> is_initialized = false;
    std::unordered_map<string, cache_entry> cached_responses;
};

} // namespace spotify
} // namespace spotifar

#endif //CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5