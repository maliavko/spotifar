#include "cache.hpp"

namespace spotifar { namespace spotify {

using namespace utils;

void from_json(const json &j, http_cache::cache_entry &e)
{
    j.at("etag").get_to(e.etag);
    j.at("body").get_to(e.body);
    
    e.cached_until = clock_t::time_point{ clock_t::duration(
        (std::int64_t)j.value("cached-until", 0LL)) };
}

void to_json(json &j, const http_cache::cache_entry &e)
{
    j = json{
        { "etag", e.etag },
        { "body", e.body },
        { "cached-until", e.cached_until.time_since_epoch().count() }
    };
}

void http_cache::start(config::settings_context &ctx)
{
    try
    {
        auto s = ctx.get_str(L"cached_http_responses", "");
        if (!s.empty())
            json::parse(s).get_to(cached_responses);
    }
    catch (const json::parse_error &ex)
    {
        log::api->error("There is an error while reading http responses "
            "cache, {}", ex.what());
    }

    is_initialized = true;
}

void http_cache::shutdown(config::settings_context &ctx)
{
    // session-only caches still can have a valid etag, so instead of removing
    // them we just invalidating `cache-until` attribute
    for (auto &[url, cache]: cached_responses)
        if (cache.is_cached_for_session())
            cache.cached_until = utils::clock_t::time_point::min();

    try
    {
        ctx.set_str(L"cached_http_responses", json(cached_responses).dump());
    }
    catch (const json::parse_error &ex)
    {
        log::api->error("There is an error while storing http responses "
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

} // namespace spotify
} // namespace spotifar