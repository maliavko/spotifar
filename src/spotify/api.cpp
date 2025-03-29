#include "api.hpp"

namespace spotifar { namespace spotify {

using namespace httplib;
using namespace utils;

const string spotify_api_url = "https://api.spotify.com";

// majority of time they are not needed, but in case of requesting
// a collection with bunch of pages we perform them asynchronously
const size_t pool_size = 12;

template<class R>
auto request_item(R &&requester, api_abstract *api) -> typename R::result_t
{
    if (requester.execute(api))
        return requester.get();
    return {};
}

// TODO: reconsider these functions
string dump_headers(const Headers &headers) {
    string s;
    char buf[BUFSIZ];

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }

    return s;
}

/// @brief clearing domain from path
static string trim_webapi_url(const string &url)
{
    if (url.starts_with(spotify_api_url))
        return url.substr(spotify_api_url.size(), url.size());
    
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

void http_logger(const Request &req, const Response &res)
{
    static const std::set<string> exclude{
        "/v1/me/player",
        "/v1/me/player/devices",
        "/v1/me/player/recently-played",
    };

    if (http::is_success(res.status))
    {
        auto url = req.path.substr(0, req.path.find("?")); // trim parameters
        if (!exclude.contains(url))
        {
            log::api->debug("A successful HTTP request has been performed (code={}): [{}] {}",
                res.status, req.method, trim_webapi_url(req.path));
        }
    }
    else
    {
        log::api->error(dump_http_error(req, res));
    }
}

api::api(): pool(pool_size)
{
    auth = std::make_unique<auth_cache>(
        this, config::get_client_id(), config::get_client_secret(),
        config::get_localhost_port());
    devices = std::make_unique<devices_cache>(this);
    history = std::make_unique<play_history>(this);
    playback = std::make_unique<playback_cache>(this);

    caches.assign({
        auth.get(), playback.get(), devices.get(), history.get(),
    });
}

api::~api()
{
    caches.clear();
}

bool api::start()
{
    utils::events::start_listening<auth_observer>(this);

    auto ctx = config::lock_settings();

    // initializing persistent caches
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->read(*ctx); });

    // initializing http responses cache
    api_responses_cache.start();

    return true;
}

void api::shutdown()
{
    utils::events::stop_listening<auth_observer>(this);

    auto ctx = config::lock_settings();
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->write(*ctx); });

    pool.purge(); // remove unfinished tasks from the queue

    api_responses_cache.shutdown();
}

void api::tick()
{
    pool.detach_loop(0ULL, caches.size(),
        [&caches = this->caches](const std::size_t idx) {
            caches[idx]->resync();
        }, BS::pr::high);
    pool.wait();
}

artist_t api::get_artist(const item_id_t &artist_id)
{
    return request_item(item_requester<artist_t>(
        std::format("/v1/artists/{}", artist_id)), this);
}

std::vector<artist_t> api::get_artists(const item_ids_t &ids)
{
    return request_item(several_items_requester<artist_t>(
        "/v1/artists", ids, 50, "artists"), this);
}

followed_artists_ptr api::get_followed_artists()
{
    return followed_artists_ptr(new followed_artists_t(
        this,
        "/v1/me/following", {
            { "type", "artist" }
        },
        "artists"
    ));
}

artist_albums_ptr api::get_artist_albums(const item_id_t &artist_id)
{
    return artist_albums_ptr(new artist_albums_t(
        this,
        std::format("/v1/artists/{}/albums", artist_id), {
            { "include_groups", "album" }
        }
    ));
}

saved_albums_ptr api::get_saved_albums()
{
    return saved_albums_ptr(new saved_albums_t(this, "/v1/me/albums"));
}

new_releases_ptr api::get_new_releases()
{
    return new_releases_ptr(new new_releases_t(
        this, "/v1/browse/new-releases", {}, "albums"));
}

std::vector<track_t> api::get_artist_top_tracks(const item_id_t &artist_id)
{
    return request_item(item_requester<std::vector<track_t>>(
        std::format("/v1/artists/{}/top-tracks", artist_id), {}, "tracks"), this);
}
    
album_t api::get_album(const item_id_t &album_id)
{
    return request_item(item_requester<album_t>(
        std::format("/v1/albums/{}", album_id)), this);
}

std::vector<album_t> api::get_albums(const item_ids_t &ids)
{
    return request_item(several_items_requester<album_t>(
        "/v1/albums", ids, 20, "albums"), this);
}

album_tracks_ptr api::get_album_tracks(const string &album_id)
{
    return album_tracks_ptr(new album_tracks_t(
        this, std::format("/v1/albums/{}/tracks", album_id)));
}

saved_tracks_ptr api::get_saved_tracks()
{
    return saved_tracks_ptr(new saved_tracks_t(this, "/v1/me/tracks"));
}

saved_playlists_ptr api::get_saved_playlists()
{
    return saved_playlists_ptr(new saved_playlists_t(this, "/v1/me/playlists"));
}

playlist_t api::get_playlist(const string &playlist_id)
{
    return request_item(item_requester<playlist_t>(
        std::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist_t::get_fields_filter() },
        }), this);
}

saved_tracks_ptr api::get_playlist_tracks(const item_id_t &playlist_id)
{
    return saved_tracks_ptr(new saved_tracks_t(
        this,
        std::format("/v1/playlists/{}/tracks", playlist_id), {
            { "additional_types", "track" },
            { "fields", std::format("items({}),next,total", saved_track_t::get_fields_filter()) },
        }));
}

bool api::check_saved_track(const string &track_id)
{
    const auto &flags = check_saved_tracks({ track_id });
    return flags.size() > 0 && flags[0];
}

std::deque<bool> api::check_saved_tracks(const item_ids_t &ids)
{
    // bloody hell, damn vector specialization with bools is not a real vector,
    // it cannot return ref to bools
    return request_item(several_items_requester<bool, std::deque<bool>>(
        "/v1/me/tracks/contains", ids, 50), this);
}

bool api::save_tracks(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto r = put("/v1/me/tracks", body.str());
    return http::is_success(r->status);
}

bool api::remove_saved_tracks(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto r = del("/v1/me/tracks", body.str());
    return http::is_success(r->status);
}

playing_queue_t api::get_playing_queue()
{
    // TODO: move to the requesters
    // auto r = get("/v1/me/player/queue");
    // if (http::is_success(r->status))
    //     return json::parse(r->body).get<playing_queue_t>();
    return {};
}

// https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
void api::start_playback(const string &context_uri, const string &track_uri,
                         int position_ms, const string &device_id)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("context_uri", context_uri);
        if (!track_uri.empty())
        {
            body.insert("position_ms", position_ms);
            body.object("offset", [&]
            {
                body.insert("uri", track_uri);
            });
        }
    });
    
    start_playback_raw(body.str(), device_id);
}

void api::start_playback(const std::vector<string> &uris, const string &device_id)
{
    assert(uris.size() > 0);

    http::json_body_builder body;

    body.object([&]
    {
        body.insert("uris", uris);
    });

    start_playback_raw(body.str(), device_id);
}

void api::start_playback(const simplified_album_t &album, const simplified_track_t &track)
{
    return start_playback(album.get_uri(), track.get_uri());
}

void api::start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track)
{
    return start_playback(playlist.get_uri(), track.get_uri());
}

void api::resume_playback(const string &device_id)
{
    return start_playback_raw("", device_id);
}

void api::toggle_playback(const string &device_id)
{
    playback->resync(true);
    auto &state = playback->get();
    if (!state.is_playing)
        return start_playback_raw("", device_id);
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
                cache.patch([](auto &v) {
                    json::Pointer("/is_playing").Set(v, false);
                });
        });
}

void api::skip_to_next(const string &device_id)
{
    pool.detach_task(
        [this, dev_id = std::as_const(device_id)]
        {
            http::json_body_builder body;

            body.object([&]
            {
                if (!dev_id.empty())
                    body.insert("device_id", dev_id);
            });

            return post("/v1/me/player/next", body.str());
        });
}

void api::skip_to_previous(const string &device_id)
{
    pool.detach_task(
        [this, dev_id = std::as_const(device_id)]
        {
            http::json_body_builder body;

            body.object([&]
            {
                if (!dev_id.empty())
                    body.insert("device_id", dev_id);
            });
            
            return post("/v1/me/player/previous", body.str());
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
                cache.patch([position_ms](auto &v) {
                    json::Pointer("/progress_ms").Set(v, position_ms);
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
                cache.patch([is_on](auto &v) {
                    json::Pointer("/shuffle_state").Set(v, is_on);
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
            auto tracks = get_album_tracks(state.context.get_item_id());
            if (tracks->fetch())
                std::transform(tracks->begin(), tracks->end(), std::back_inserter(uris),
                                [](const auto &t) { return t.get_uri(); });
        }
        else if (state.context.is_playlist())
        {
            auto tracks = get_playlist_tracks(state.context.get_item_id());
            if (tracks->fetch())
                std::transform(tracks->begin(), tracks->end(), std::back_inserter(uris),
                               [](const auto &t) { return t.get_uri(); });
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
                cache.patch([mode](auto &d) {
                    json::Pointer("/repeat_state").Set(d, mode);
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
                cache.patch([volume_percent](auto &d) {
                    json::Pointer("/device/volume_percent").Set(d, volume_percent);
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
            http::json_body_builder body;

            body.object([&]
            {
                body.insert("device_ids", { dev_id });
                body.insert("play", start_playing);
            });

            auto res = put("/v1/me/player", body.str());
            // TODO: handle errors
        });
}

void api::start_playback_raw(const string &body, const string &device_id)
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
                cache.patch([](auto &d) {
                    json::Pointer("/is_playing").Set(d, true);
                });
        });
}

bool api::is_request_cached(const string &url) const
{
    string u = trim_webapi_url(url);
    return api_responses_cache.is_cached(u) && api_responses_cache.get(u).is_valid();
}

httplib::Result api::get(const string &request_url, clock_t::duration cache_for)
{
    // TODO: the method is called in separate threads while populating async collections,
    // sometimes it races for `api_responses_cache`. It needs to introduce some mutex here
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

    auto r = get_client()->Get(request_url, {{ "If-None-Match", cached_etag }});
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
            const auto &cache = api_responses_cache.get(url);

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

httplib::Result api::put(const string &request_url, const string &body)
{
    httplib::Result res;
    auto client = get_client();

    if (!body.empty())
        res = client->Put(request_url, body, "application/json");
    else
        res = client->Put(request_url);
    
    if (http::is_success(res->status))
    {
        // invalidate all cached GET responses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

httplib::Result api::del(const string &request_url, const string &body)
{
    auto res = get_client()->Delete(request_url, body, "application/json");
    
    if (http::is_success(res->status))
    {
        // invalidate all the cached repsonses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

httplib::Result api::post(const string &request_url, const string &body)
{
    auto res = get_client()->Post(request_url, body, "application/json");
    
    if (http::is_success(res->status))
    {
        // invalidate all the cached repsonses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

std::shared_ptr<httplib::Client> api::get_client() const
{
    auto client = std::make_shared<httplib::Client>(spotify_api_url);

    client->set_logger(http_logger);    
    client->set_bearer_token_auth(auth->get_access_token());
    client->set_default_headers({
        {"Content-Type", "application/json; charset=utf-8"},
    });

    return client;
}

void api::on_auth_status_changed(const auth_t &auth)
{
    // pick up the some device for playback
    devices->pick_up_device();
}

} // namespace spotify
} // namespace spotifar