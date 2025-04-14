#include "cache.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

using namespace utils;

string get_cache_filename()
{
    return std::format("{}\\responses.cache", utils::to_string(config::get_plugin_data_folder()));
}

void from_json(const json::Value &j, http_cache::cache_entry &e)
{
    e.etag = j["etag"].GetString();
    e.body = j["body"].GetString();

    std::int64_t cached_until = 0LL;
    if (j.HasMember("cached-until"))
        cached_until = j["cached-until"].GetInt64();
    
    e.cached_until = clock_t::time_point{ clock_t::duration(cached_until) };
}

void to_json(json::Value &result, const http_cache::cache_entry &e, json::Allocator allocator)
{
    result = json::Value(json::kObjectType);

    result.AddMember("etag", json::Value(e.etag, allocator), allocator);
    result.AddMember("body", json::Value(e.body, allocator), allocator);
    result.AddMember("cached-until",
        json::Value(e.cached_until.time_since_epoch().count()), allocator);
}

void http_cache::start()
{
    try
    {
        string filepath = get_cache_filename();

        HANDLE file = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        
        // there is no cache for reading
        if (file == INVALID_HANDLE_VALUE)
            return;

        // create a reading "file mapping" for the intire file
        HANDLE fmap = CreateFileMapping(file, 0, PAGE_READONLY, 0, 0, 0);
        if (fmap == INVALID_HANDLE_VALUE)
            return;

        // map the file into memory
        const char *buffer = (const char*)MapViewOfFile(fmap, FILE_MAP_READ, 0, 0, 0);
        if (buffer == nullptr)
        {
            log::global->warn("There is an unexpected empty cache file. "
                "Skipping cache initialization");
            return;
        }

        // parsing json cache
        json::parse_to(buffer, cached_responses);

        // close all handles
        UnmapViewOfFile(buffer);
        CloseHandle(fmap);
        CloseHandle(file);
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error while reading http responses "
            "cache, {}", ex.what());
    }

    is_initialized = true;
}

void http_cache::shutdown()
{
    // session-only caches still can have a valid etag, so instead of removing
    // them we just invalidating `cache-until` attribute
    for (auto &[url, cache]: cached_responses)
        if (cache.is_cached_for_session())
            cache.cached_until = utils::clock_t::time_point::min();

    try
    {
        string filepath = get_cache_filename();

        auto sbuffer = json::dump(cached_responses);
        DWORD totalSize = (DWORD)sbuffer->GetLength();

        HANDLE file = CreateFileA(filepath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0,
            CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, 0);
            
        if (file == INVALID_HANDLE_VALUE)
        {
            log::global->error("An error occured while opening a file for dumping an "
                "http cache, {}", get_last_system_error());
            return;
        }

        // create a "file mapping" for the file. Grows it to toalSize
        HANDLE fmap = CreateFileMapping(file, 0, PAGE_READWRITE, 0, totalSize, 0);
        if (file == INVALID_HANDLE_VALUE)
        {
            log::global->error("An error occured while creating a mapped cache file, {}",
                get_last_system_error());
            return;
        }

        // map the file into memory
        void *buffer = MapViewOfFile(fmap, FILE_MAP_WRITE, 0, 0, totalSize);

        std::memcpy(buffer, sbuffer->GetString(), totalSize);

        // close all handles etc.
        UnmapViewOfFile(buffer);
        CloseHandle(fmap);
        CloseHandle(file);
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error while storing http responses "
            "cache, {}", ex.what());
    }
}

bool http_cache::is_cached(const string &url) const
{
    std::lock_guard lock(guard);
    return cached_responses.contains(url);
}

const http_cache::cache_entry& http_cache::get(const string &url) const
{
    std::lock_guard lock(guard);
    return cached_responses.at(url);
}

void http_cache::store(const string &url, string body, const string &etag, clock_t::duration cache_for)
{
    clock_t::time_point cached_until = clock_t::now() + cache_for;

    if (cache_for == http::session)
        cached_until = clock_t::time_point::max();

    {
        std::lock_guard lock(guard);
        cached_responses[url] = { etag, body, cached_until };
    }
}

void http_cache::store(const string &url, const cache_entry &entry)
{
    std::lock_guard lock(guard);
    cached_responses[url] = entry;
}

void http_cache::invalidate(const string &url_part)
{
    std::lock_guard lock(guard);
    for (auto it = cached_responses.begin(); it != cached_responses.end();)
    {
        auto pos = it->first.find(url_part);
        if (pos != string::npos)
            cached_responses.erase(it++);
        else
            ++it;
    }
}

void http_cache::clear_all()
{
    std::lock_guard lock(guard);
    cached_responses.clear();
}

} // namespace spotify
} // namespace spotifar