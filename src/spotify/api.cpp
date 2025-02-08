#include "api.hpp"

namespace spotifar { namespace spotify {

using namespace httplib;
using namespace utils;

bool is_success(int status)
{
    return status == OK_200 || status == NoContent_204 || status == NotModified_304;
}

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

const string SPOTIFY_API_URL = "https://api.spotify.com";

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
            if (is_success(res.status))
            {
                if (!exclude.contains(req.path))
                {
                    // clearing domain from path, just for log readability
                    auto req_path = req.path;
                    if (req_path.starts_with(SPOTIFY_API_URL))
                        req_path.erase(req_path.begin(), req_path.begin() + SPOTIFY_API_URL.size());
                    
                    log::api->debug("A successful HTTP request has been performed (code={}): [{}] {}",
                                    res.status, req.method, req_path);
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
    library = std::make_unique<LibraryCache>(this);

    caches.assign({
        auth.get(), playback.get(), devices.get(), library.get()//, history.get(),
    });
}

api::~api()
{
    caches.clear();
}

bool api::start()
{
    auto ctx = config::lock_settings();
    for (auto &c: caches)
        c->read(*ctx);

    // TODO: this caches is definitely a tmp solution
    auto s = ctx->get_str(L"responses", "");
    if (!s.empty())
        responses_cache = json::parse(s);

    return true;
}

void api::shutdown()
{
    auto ctx = config::lock_settings();
    for (auto &c: caches)
        c->write(*ctx);

    ctx->set_str(L"responses", responses_cache.dump());
    
    pool.purge();

    ObserverManager::clear<BaseObserverProtocol>();
}

void api::tick()
{
    pool.detach_loop(0ULL, caches.size(),
        [&caches = this->caches](const std::size_t idx) {
            caches[idx]->resync();
        }, BS::pr::high);
    pool.wait();
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
            { "offset", { "uri", track_uri } },
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

void api::start_playback(const SimplifiedAlbum &album, const SimplifiedTrack &track)
{
    return start_playback(album.get_uri(), track.get_uri());
}

void api::start_playback(const SimplifiedPlaylist &playlist, const SimplifiedTrack &track)
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
        [&c = this->client, &cache = *playback, dev_id = std::as_const(device_id)]
        {
            Params params = {};
            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = c.Put(append_query_params("/v1/me/player/pause", params));
            if (is_success(res))
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
        [&c = this->client, position_ms, &cache = *playback, dev_id = std::as_const(device_id)]
        {
            Params params = {
                { "position_ms", std::to_string(position_ms) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = c.Put(append_query_params("/v1/me/player/seek", params));
            if (is_success(res->status))
                cache.patch({
                    { "progress_ms", position_ms }
                });
        });
}

void api::toggle_shuffle(bool is_on, const string &device_id)
{
    pool.detach_task(
        [&c = this->client, is_on, &cache = *playback, dev_id = std::as_const(device_id)]
        {
            Params params = {
                { "state", is_on ? "true" : "false" },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });
            
            auto res = c.Put(append_query_params("/v1/me/player/shuffle", params));
            if (is_success(res->status))
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
        [&c = this->client, mode, &cache = *playback, dev_id = std::as_const(device_id)]
        {
            Params params = {
                { "state", mode },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = c.Put(append_query_params("/v1/me/player/repeat", params));
            if (is_success(res->status))
                cache.patch({
                    { "repeat_state", mode }
                });
        });
}

void api::set_playback_volume(int volume_percent, const string &device_id)
{
    pool.detach_task(
        [&c = this->client, volume_percent, &cache = *playback, dev_id = std::as_const(device_id)]
        {
            Params params = {
                { "volume_percent", std::to_string(volume_percent) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = c.Put(append_query_params("/v1/me/player/volume", params));
            if (is_success(res->status))
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
        return log::api->warn("The given device is already active {}", device_it->to_str());
    
    pool.detach_task(
        [&c = this->client, start_playing, dev_id = std::as_const(device_id)]
        {
            json body{
                { "device_ids", { dev_id } },
                { "play", start_playing },
            };

            auto res = c.Put("/v1/me/player", body.dump(), "application/json");
            // TODO: handle errors
        });
}

SimplifiedTracksT api::get_album_tracks(const string &album_id)
{
    SimplifiedTracksT result;
    
    json request_url = httplib::append_query_params(
        std::format("/v1/albums/{}/tracks", album_id), {
            { "limit", std::to_string(50) },
        });
        
    do
    {
        if (auto r = get(request_url))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &tracks = data["items"].get<SimplifiedTracksT>();
            result.insert(result.end(), tracks.begin(), tracks.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

PlaylistTracksT api::get_playlist_tracks(const string &playlist_id)
{
    PlaylistTracksT result;
    
    static string fields = std::format("items({}),next", PlaylistTrack::get_fields_filter());
    json request_url = httplib::append_query_params(
        std::format("/v1/playlists/{}/tracks", playlist_id), {
            { "limit", std::to_string(50) },
            { "additional_types", "track" },
            { "fields", fields },
        });

    int idx = 0;
        
    do
    {
        if (auto r = get(request_url))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &tracks = data["items"].get<PlaylistTracksT>();
            result.insert(result.end(), tracks.begin(), tracks.end());
        }
        if (idx++ > 3)
            break; // TODO: remove, tmp code to speed up a development
    }
    while (!request_url.is_null());

    return result;
}

TracksT api::get_artist_top_tracks(const string &artist_id)
{
    json request_url = std::format("/v1/artists/{}/top-tracks", artist_id);
    if (auto r = get(request_url))
    {
        json data = json::parse(r->body);
        return data["tracks"].get<TracksT>();
    }
    return {};
}

AlbumsCollection api::get_albums(const string &artist_id)
{
    AlbumsCollection albums;

    Params params = {
        { "limit", "50" },
        { "offset", "0" },
        { "include_groups", "album" }
    };

    string request_url = append_query_params(
        std::format("/v1/artists/{}/albums", artist_id), params);

    do
    {
        //auto r = client.Get(request_url);
        auto r = get(request_url);

        json data = json::parse(r->body);
        for (json& aj : data["items"])
        {
            auto a = aj.get<SimplifiedAlbum>();
            albums[a.id] = a;
        }

        json next = data["next"];
        if (next.is_null())
            break;
        
        next.get_to(request_url);
    } while (1);

    return albums;
}

PlaylistsCollection api::get_playlists()
{
    PlaylistsCollection playlists;

    Params params = {
        { "limit", "50" },
        { "offset", "0" },
    };

    string request_url = append_query_params("/v1/me/playlists", params);

    do
    {
        auto r = client.Get(request_url);

        json data = json::parse(r->body);
        for (json& aj : data["items"])
        {
            auto p = aj.get<SimplifiedPlaylist>();
            playlists[p.id] = p;
        }

        json next = data["next"];
        if (next.is_null())
            break;
        
        next.get_to(request_url);
    } while (1);

    return playlists;
}

void api::start_playback(const json &body, const string &device_id)
{
    Params params = {};
    if (!device_id.empty())
        params.insert({ "device_id", device_id });

    pool.detach_task(
        [
            &c = this->client, &cache = *playback, dev_id = std::as_const(device_id),
            request_url = append_query_params("/v1/me/player/play", params), body
        ]
        {
            Result res;
            if (body.empty())
                res = c.Put(request_url);
            else
                res = c.Put(request_url, body.dump(), "application/json");

            if (is_success(res))
                cache.patch({
                    { "is_playing", true }
                });
        });
}

httplib::Result api::get(const string &request_url, clock_t::duration cache_for)
{
    auto &cache = responses_cache[request_url];

    string cached_etag = "";
    if (!cache.is_null())
    {
        auto cached_until = clock_t::time_point{ clock_t::duration(
            (std::int64_t)cache.value("cached-until", 0LL)) };

        // if caching time is specified and still valid - returning cached result
        if (clock_t::now() < cached_until)
        {
            httplib::Result res(std::make_unique<httplib::Response>(), httplib::Error::Success);
            res->status = httplib::OK_200;
            res->body = cache.value("body", "");
            return res;
        }

        cached_etag = cache.value("etag", "");
    }

    if (auto r = client.Get(request_url, {{ "If-None-Match", cached_etag }}))
    {
        // collecting valid-until timestamp
        if (cache_for > clock_t::duration::zero())
        {
            cache["cached-until"] = (clock_t::now() + cache_for).time_since_epoch().count();
        }

        if (r->status == httplib::OK_200)
        {
            if (r->has_header("etag"))
                cache["etag"] = r->get_header_value("etag");
        }
        else if (r->status == httplib::NotModified_304)
        {
            r->body = cache.value("body", "");
        }

        // if either cache-until or etag is present - caching body as well
        if (!cache.empty())
            cache["body"] = r->body;
        
        return r;
    }
    return httplib::Result();
}

void api::set_bearer_token_auth(const string &token)
{
    client.set_bearer_token_auth(token);
}

} // namespace spotify
} // namespace spotifar