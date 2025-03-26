#include "cache.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

using namespace utils;

void from_rapidjson(const rapidjson::Value &j, http_cache::cache_entry &e)
{
    e.etag = j["etag"].GetString();
    e.body = j["body"].GetString();

    std::int64_t cached_until = 0LL;
    if (j.HasMember("cached-until"))
        cached_until = j["cached-until"].GetInt64();
    
    e.cached_until = clock_t::time_point{ clock_t::duration(cached_until) };
}

void to_json(json &j, const http_cache::cache_entry &e)
{
    j = json{
        { "etag", e.etag },
        { "body", e.body },
        { "cached-until", e.cached_until.time_since_epoch().count() }
    };
}

void http_cache::start()
{
    try
    {
        string filepath = get_cache_filename();
        if (!std::filesystem::exists(filepath))
        {
            std::ofstream file(filepath);
            file << "{}";
        }

        std::error_code error;
        mio::mmap_source mmap = mio::make_mmap_source(filepath, error);
        if (error)
        {
            log::global->error("An error occured while mapping cache file, a cache file "
                "initialization is skipped, ({}) {}", error.value(), error.message());
            return;
        }

        rapidjson::Document document;
        rapidjson::Value &data = document.Parse(mmap.data());
        
        from_rapidjson(data, cached_responses);

        mmap.unmap();
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
        std::ofstream file(get_cache_filename());
        file << json(cached_responses).dump();
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error while storing http responses "
            "cache, {}", ex.what());
    }
}

bool http_cache::is_cached(const string &url) const
{
    return cached_responses.contains(url);
}

const http_cache::cache_entry& http_cache::get(const string &url) const
{
    return cached_responses.at(url);
}

void http_cache::store(const string &url, string body, const string &etag, clock_t::duration cache_for)
{
    clock_t::time_point cached_until = clock_t::now() + cache_for;

    if (cache_for == http::session)
        cached_until = clock_t::time_point::max();

    cached_responses[url] = { etag, body, cached_until };
}

void http_cache::store(const string &url, const cache_entry &entry)
{
    cached_responses[url] = entry;
}

void http_cache::invalidate(const string &url_part)
{
    for (auto it = cached_responses.begin(); it != cached_responses.end();)
    {
        auto pos = it->first.find(url_part);
        if (pos != string::npos)
            cached_responses.erase(it++);
        else
            ++it;
    }
}

string http_cache::get_cache_filename()
{
    return std::format("{}\\responses.cache", utils::to_string(config::get_plugin_data_folder()));
}

} // namespace spotify
} // namespace spotifar