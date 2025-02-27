#include "api.hpp"

namespace spotifar { namespace spotify {

using namespace httplib;
using namespace utils;

const string SPOTIFY_API_URL = "https://api.spotify.com";

// TODO: reconsider these functions
string dump_headers(const Headers &headers) {
    string s;
    char buf[BUFSIZ];

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }

    return s;
}

/// @brief clearing domain from path
static string trim_webapi_url(const string &url)
{
    if (url.starts_with(SPOTIFY_API_URL))
        return url.substr(SPOTIFY_API_URL.size(), url.size());
    return url;
}

string dump_http_error(const Request &req, const Response &res)
{
    std::stringstream ss, query;
    
    for (auto it = req.params.begin(); it != req.params.end(); ++it)
        query << std::format("{}{}={}", (it == req.params.begin()) ? '?' : '&', it->first, it->second);

    ss  << "An error occured while making an http request: "
        << std::format("{} {} {}", req.method, req.version, req.path) << query.str() << std::endl;

    //ss << dump_headers(req.headers);

    ss << std::endl << "A response received: " << std::endl;
    ss << std::format("{} {}", res.status, res.version) << std::endl;

    //ss << dump_headers(res.headers) << std::endl;

    if (!res.body.empty())
        ss << res.body << std::endl;

    return ss.str();
}

api::api():
    client(SPOTIFY_API_URL),
    pool(8)
{
    static const std::set<string> exclude{
        "/v1/me/player",
        "/v1/me/player/devices",
    };

    client.set_logger(
        [this](const Request &req, const Response &res)
        {
            if (http::is_success(res.status))
            {
                if (!exclude.contains(req.path))
                {
                    log::api->debug("A successful HTTP request has been performed (code={}): [{}] {}",
                                    res.status, req.method, trim_webapi_url(req.path));
                }
            }
            else
                log::api->error(dump_http_error(req, res));
        });
    
    client.set_default_headers(Headers{
        {"Content-Type", "application/json; charset=utf-8"},
    });

    auth = std::make_unique<auth_cache>(
        this, config::get_client_id(), config::get_client_secret(),
        config::get_localhost_port());
    devices = std::make_unique<devices_cache>(this);
    // history = std::make_unique<PlayedHistory>(this);
    playback = std::make_unique<playback_cache>(this);

    caches.assign({
        auth.get(), playback.get(), devices.get(), //, history.get(),
    });
}

api::~api()
{
    caches.clear();
}

bool api::start()
{
    ObserverManager::subscribe<auth_observer>(this);

    auto ctx = config::lock_settings();

    // initializing persistent caches
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->read(*ctx); });

    // initializing http responses cache
    pool.detach_task([this, ctx] { api_responses_cache.start(*ctx); }, BS::pr::highest);

    return true;
}

void api::shutdown()
{
    ObserverManager::unsubscribe<auth_observer>(this);

    auto ctx = config::lock_settings();
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->write(*ctx); });

    pool.purge(); // remove unfinished tasks from the queue

    api_responses_cache.shutdown(*ctx);
}

void api::tick()
{
    pool.detach_loop(0ULL, caches.size(),
        [&caches = this->caches](const std::size_t idx)
        {
            caches[idx]->resync();
        }, BS::pr::high);
    pool.wait();
}

artist api::get_artist(const string &artist_id)
{
    auto r = get(std::format("/v1/artists/{}", artist_id), utils::http::session);
    if (http::is_success(r->status))
        return json::parse(r->body).get<artist>();
    return artist();
}

tracks_t api::get_artist_top_tracks(const string &artist_id)
{
    auto r = get(std::format("/v1/artists/{}/top-tracks", artist_id), utils::http::session);
    if (http::is_success(r->status))
    {
        json data = json::parse(r->body);
        return data["tracks"].get<tracks_t>();
    }
    return {};
}
    
album api::get_album(const string &album_id)
{
    auto r = get(std::format("/v1/albums/{}", album_id), utils::http::session);
    if (http::is_success(r->status))
        return json::parse(r->body).get<album>();
    return album();
}

simplified_tracks_t api::get_album_tracks(const string &album_id)
{
    simplified_tracks_t result;
    
    json request_url = httplib::append_query_params(
        std::format("/v1/albums/{}/tracks", album_id), {
            { "limit", std::to_string(50) },
        });
        
    do
    {
        auto r = get(request_url, utils::http::session);
        if (http::is_success(r->status))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &tracks = data["items"].get<simplified_tracks_t>();
            result.insert(result.end(), tracks.begin(), tracks.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

playlist api::get_playlist(const string &playlist_id)
{
    json request_url = httplib::append_query_params(
        std::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist::get_fields_filter() },
        });

    auto r = get(request_url, utils::http::session);
    if (http::is_success(r->status))
        return json::parse(r->body).get<playlist>();
    return playlist();
}

playlist_tracks_t api::get_playlist_tracks(const string &playlist_id)
{
    playlist_tracks_t result;
    
    static string fields = std::format("items({}),next,total", playlist_track::get_fields_filter());
    json request_url = httplib::append_query_params(
        std::format("/v1/playlists/{}/tracks", playlist_id), {
            { "limit", std::to_string(50) },
            { "additional_types", "track" },
            { "fields", fields },
        });

    do
    {
        auto r = get(request_url, utils::http::session);
        if (utils::http::is_success(r->status))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &entries = data["items"].get<playlist_tracks_t>();
            result.insert(result.end(), entries.begin(), entries.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

bool api::check_saved_track(const string &track_id)
{
    auto flags = check_saved_tracks({ track_id });
    if (flags.size() > 0)
        return flags[0];
    return false;
}

std::vector<bool> api::check_saved_tracks(const std::vector<string> &ids)
{
    auto request_url = httplib::append_query_params(
        "/v1/me/tracks/contains", {{ "ids", utils::string_join(ids, ",") }});
    auto r = get(request_url, utils::http::session);
    if (http::is_success(r->status))
    {
        std::vector<bool> result;
        json::parse(r->body).get_to(result);
        
        return result;
        
    }
    return {};
}

bool api::save_tracks(const std::vector<string> &ids)
{
    auto r = put("/v1/me/tracks", json{{ "ids", ids }});
    return http::is_success(r->status);
}

bool api::remove_saved_tracks(const std::vector<string> &ids)
{
    auto r = del("/v1/me/tracks", json{{ "ids", ids }});
    return http::is_success(r->status);
}

playing_queue api::get_playing_queue()
{
    auto r = get("/v1/me/player/queue");
    if (http::is_success(r->status))
        return json::parse(r->body).get<playing_queue>();
    return playing_queue();
}

// https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
void api::start_playback(const string &context_uri, const string &track_uri,
                         int position_ms, const string &device_id)
{
    json body{
        { "context_uri", context_uri },
    };

    if (!track_uri.empty())
        body.update({
            { "offset", {{ "uri", track_uri }} },
            { "position_ms", position_ms },
        });
    
    start_playback(body, device_id);
}

void api::start_playback(const std::vector<string> &uris, const string &device_id)
{
    assert(uris.size() > 0);

    json body{
        { "uris", uris }
    };
    
    start_playback(body, device_id);
}

void api::start_playback(const simplified_album &album, const simplified_track &track)
{
    return start_playback(album.get_uri(), track.get_uri());
}

void api::start_playback(const simplified_playlist &playlist, const simplified_track &track)
{
    return start_playback(playlist.get_uri(), track.get_uri());
}

void api::resume_playback(const string &device_id)
{
    return start_playback(json(), device_id);
}

void api::toggle_playback(const string &device_id)
{
    playback->resync();
    auto &state = playback->get();
    if (!state.is_playing)
        return start_playback(json(), device_id);
    else
        return pause_playback(device_id);
}

void api::pause_playback(const string &device_id)
{
    pool.detach_task(
        [&cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {};
            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/pause", params));
            if (http::is_success(res))
                cache.patch({
                    { "is_playing", false }
                });
        });
}

void api::skip_to_next(const string &device_id)
{
    pool.detach_task(
        [&c = this->client, dev_id = std::as_const(device_id)]
        {
            Params params = {};
            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            return c.Post("/v1/me/player/next", params);
        });
}

void api::skip_to_previous(const string &device_id)
{
    pool.detach_task(
        [&c = this->client, dev_id = std::as_const(device_id)]
        {
            Params params = {};
            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });
            
            return c.Post("/v1/me/player/previous", params);
        });
}

void api::seek_to_position(int position_ms, const string &device_id)
{
    pool.detach_task(
        [position_ms, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "position_ms", std::to_string(position_ms) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/seek", params));
            if (http::is_success(res->status))
                cache.patch({
                    { "progress_ms", position_ms }
                });
        });
}

void api::toggle_shuffle(bool is_on, const string &device_id)
{
    pool.detach_task(
        [is_on, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "state", is_on ? "true" : "false" },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });
            
            auto res = put(append_query_params("/v1/me/player/shuffle", params));
            if (http::is_success(res->status))
                cache.patch({
                    { "shuffle_state", is_on }
                });
        });
}

void api::toggle_shuffle_plus(bool is_on)
{
    if (is_on)
    {
        std::vector<string> uris;
        auto &state = get_playback_state();
        if (state.context.is_album())
        {
            const auto &tracks = get_album_tracks(state.context.get_item_id());
            std::transform(tracks.begin(), tracks.end(), std::back_inserter(uris),
                            [](const auto &t) { return t.get_uri(); });
        }
        else if (state.context.is_playlist())
        {
            const auto &tracks = get_playlist_tracks(state.context.get_item_id());
            std::transform(tracks.begin(), tracks.end(), std::back_inserter(uris),
                            [](const auto &t) { return t.track.get_uri(); });
        }
        else if (state.context.is_artist())
        {
            const auto &tracks = get_artist_top_tracks(state.context.get_item_id());
            std::transform(tracks.begin(), tracks.end(), std::back_inserter(uris),
                            [](const auto &t) { return t.get_uri(); });
        }
        playback->activate_super_shuffle(uris);
    }
}

void api::set_repeat_state(const string &mode, const string &device_id)
{
    pool.detach_task(
        [mode, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "state", mode },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/repeat", params));
            if (http::is_success(res->status))
                cache.patch({
                    { "repeat_state", mode }
                });
        });
}

void api::set_playback_volume(int volume_percent, const string &device_id)
{
    pool.detach_task(
        [volume_percent, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "volume_percent", std::to_string(volume_percent) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/volume", params));
            if (http::is_success(res->status))
                cache.patch({
                    { "device", {
                        { "volume_percent", volume_percent }
                    } }
                });
        });
}

void api::transfer_playback(const string &device_id, bool start_playing)
{   
    auto devices = get_available_devices();
    auto device_it = std::find_if(devices.begin(), devices.end(),
        [&device_id](auto &d) { return d.id == device_id; });

    if (device_it == devices.end())
        return log::api->error("There is no devices with the given id={}", device_id);

    if (device_it->is_active)
        return log::api->warn("The given device is already active, {}", device_it->to_str());
    
    pool.detach_task(
        [this, start_playing, dev_id = std::as_const(device_id)]
        {
            json body{
                { "device_ids", { dev_id } },
                { "play", start_playing },
            };

            auto res = put("/v1/me/player", body);
            // TODO: handle errors
        });
}

void api::start_playback(const json &body, const string &device_id)
{
    Params params = {};
    if (!device_id.empty())
        params.insert({ "device_id", device_id });

    pool.detach_task(
        [
            this, &cache = *playback, dev_id = std::as_const(device_id),
            request_url = append_query_params("/v1/me/player/play", params), body
        ]
        {
            auto res = put(request_url, body);
            if (http::is_success(res))
                cache.patch({
                    { "is_playing", true }
                });
        });
}

void api::clear_http_cache()
{
    api_responses_cache.clear_all();
    log::api->debug("Clearning caches");
}

httplib::Result api::get(const string &request_url, clock_t::duration cache_for)
{
    using namespace httplib;

    string cached_etag = "";
    string url = trim_webapi_url(request_url);

    // we have a cache for the requested url and it is still valid
    if (api_responses_cache.is_cached(url))
    {
        auto cache = api_responses_cache.get(url);
        if (cache.is_valid())
        {
            Result res(std::make_unique<Response>(), Error::Success);
            res->status = OK_200;
            res->body = cache.body;
            return res;
        }

        cached_etag = cache.etag;
    }

    auto r = client.Get(request_url, {{ "If-None-Match", cached_etag }});
    if (http::is_success(r->status))
    {
        if (r->status == OK_200)
        {
            auto etag = r->get_header_value("etag", "");

            // caching only requests which have ETag or `cache-for` instruction
            if (!etag.empty() || cache_for > clock_t::duration::zero())
                api_responses_cache.store(url, r->body, etag, cache_for);
        }
        else if (r->status == NotModified_304)
        {
            auto cache = api_responses_cache.get(url);

            // replacing empty body with the cached one, so the client
            // does not see the difference
            r->body = cache.body;

            // the response is still valid, so caching for a session or any other
            // time if needed
            if (cache_for != clock_t::duration::zero())
                api_responses_cache.store(url, cache.body, cache.etag, cache_for);
        }
    }
    else if (r->status == TooManyRequests_429)
    {
        // TODO: I hope it will not go into recursion one day, sorry guys
        std::this_thread::sleep_for(1.5s);
        log::api->debug("Postponing an api http request due to reached rate limits, {}", request_url);
        return get(request_url, cache_for);
    }

    return r;
}

httplib::Result api::put(const string &request_url, const json &body)
{
    httplib::Result res;

    if (!body.empty())
        res = client.Put(request_url, body.dump(), "application/json");
    else
        res = client.Put(request_url);
    
    if (http::is_success(res->status))
    {
        // invalidate all cached GET responses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

httplib::Result api::del(const string &request_url, const json &body)
{
    auto res = client.Delete(request_url, body.dump(), "application/json");
    
    if (http::is_success(res->status))
    {
        // invalidate all the cached repsonses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

void api::on_auth_status_changed(const spotify::auth &auth)
{
    // set up current session's valid access token
    client.set_bearer_token_auth(auth.access_token);

    // pick up the some device for playback
    devices->pick_up_device();
}

} // namespace spotify
} // namespace spotifar