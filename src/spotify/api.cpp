#include "api.hpp"
#include "requesters.hpp"
#include "library.hpp"
#include "releases.hpp"
#include "auth.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "releases.hpp"
#include "history.hpp"
#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace spotify {

namespace fs = std::filesystem;
using namespace httplib;
using namespace utils;

const string spotify_api_url = "https://api.spotify.com";

// std::random_device rd;                  // get a random seed from hardware
// std::mt19937 gen(rd());                 // Mersenne Twister PRNG seeded with rd
// std::bernoulli_distribution d(0.85);    // 50% chance for true, 50% for false

// a helper-function to avoid copy-pasting. Performs an execution of a given
// requester, checks the result and returns it back
// @tparam R item_requester type
template<class R>
static auto request_item(R &&requester, api_weak_ptr_t api) -> typename R::result_t
{
    if (requester.execute(api))
        return requester.get();
    
    dispatch_event(&api_requests_observer::on_collection_fetching_failed, get_fetching_error(&requester));

    return {};
}

static httplib::Result get_rate_limited_response(const utils::clock_t::time_point &expires_at)
{
    Result res(std::make_unique<Response>(), Error::Success);

    http::json_body_builder body;
    body.object([&]
    {
        body.object("error", [&]
        {
            body.insert("message", utils::format("expires at {}", utils::format("{:%T}", expires_at)));
        });
    });

    res->status = TooManyRequests_429;
    res->body = body.str();

    return res;
}

//----------------------------------------------------------------------------------------------

/// @brief Returns an endpoint's name by the given http url. In practise it is the first
/// part of the url, excep the API versino specificator "v1"
static string get_endpoint_name(const string &url)
{
    static auto pattern = std::regex("/v1/(.*?)/.*");

    std::smatch match;
    if (std::regex_search(url, match, pattern) && match.size() > 1)
        return match[1].str();
    return "";
}

void endpoint_guard::wait(std::function<bool()> predicate)
{
    if (is_rate_limited())
    {
        // if the endpoint is busy, the accessing thread is put into the waiting
        // queue until the expiration time or `predicate` is `true`
        std::unique_lock<std::mutex> thread_lock(guard);
        cv.wait_until(thread_lock, expires_at, predicate);
    }
}

void endpoint_guard::set_expires_at(const clock_t::time_point &tp)
{
    expires_at = tp;
}

const utils::clock_t::time_point& endpoint_guard::get_expires_at() const
{
    return expires_at;
}

endpoint_guard& api::get_endpoint(const string &url)
{
    const auto &ep_name = get_endpoint_name(url);

    std::mutex m;
    std::lock_guard guard(m);

    // we create a new guard if there is no such exists for the given `ep_name`
    const auto it = guards.try_emplace(ep_name, ep_name);
    return it.first->second;
}


//----------------------------------------------------------------------------------------------
api::api(): requests_pool(5), resyncs_pool(3)
{
    api_responses_cache = std::make_unique<http_cache>();
}

api::~api()
{
    api_responses_cache.reset();
}

bool api::start()
{
    // the order matters
    auth = std::make_unique<auth_cache>(this, config::get_client_id(), config::get_client_secret(), config::get_localhost_port());
    library = std::make_unique<spotify::library>(this);
    devices = std::make_unique<devices_cache>(this);
    history = std::make_unique<play_history>(this);
    playback = std::make_unique<playback_cache>(this);
    releases = std::make_unique<recent_releases>(this);

    caches.assign({ auth.get(), playback.get(), devices.get(), history.get(), releases.get(), library.get() });

    auto ctx = config::lock_settings();

    // initializing persistent caches
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->read(*ctx); });

    // initializing http responses cache
    api_responses_cache->start();

    // for debugging, marks some endpoints as rate limited from the start of the app
    // auto it = guards.try_emplace("me", "me");
    // it.first->second.set_expires_at(utils::clock_t::now() + 60min);

    return true;
}

void api::shutdown()
{
    cancel_pending_requests(false);

    if (auto ctx = config::lock_settings())
        std::for_each(caches.begin(), caches.end(), [ctx](auto &c){ c->shutdown(*ctx); });

    // removing unfinished tasks from the queue
    requests_pool.purge();
    resyncs_pool.purge();

    api_responses_cache->shutdown();
    
    caches.clear();

    // NOTE: there is a feeling, that killing the obejcts here leads
    // to hanging process, due to the caches access data mutex dying,
    // but the main thread wait for all of them to finish properly
    /*auth.reset();
    library.reset();
    devices.reset();
    history.reset();
    playback.reset();
    releases.reset();*/
}

void api::tick()
{
    auto future = resyncs_pool.submit_loop<size_t>(0, caches.size(),
        [&caches = this->caches](const std::size_t idx)
        {
            if (idx < caches.size() && caches[idx])
                caches[idx]->resync();
        });
    future.get();
}

const playback_cache::data_t& api::get_playback_state(bool force_resync)
{
    playback->resync(force_resync);
    return playback->get();
}

library_interface* api::get_library()
{
    return library.get();
};

recent_releases_interface* api::get_releases()
{
    return releases.get();
};

auth_cache_interface* api::get_auth_cache()
{
    return auth.get();
};

devices_cache_interface* api::get_devices_cache(bool resync)
{
    devices->resync(resync);
    return devices.get();
};

const play_history::data_t& api::get_play_history(bool force_resync)
{
    history->resync(force_resync);
    return history->get();
}

artist_t api::get_artist(const item_id_t &artist_id)
{
    return request_item(item_requester<artist_t, 1, std::chrono::weeks>(
        utils::format("/v1/artists/{}", artist_id)), get_ptr());
}

std::vector<artist_t> api::get_artists(const item_ids_t &ids)
{
    return request_item(several_items_requester<artist_t, 1, std::chrono::weeks>(
        "/v1/artists", ids, 50, "artists"), get_ptr());
}

artist_albums_ptr api::get_artist_albums(const item_id_t &artist_id, const std::vector<string> &groups)
{
    return artist_albums_ptr(new artist_albums_t(
        get_ptr(),
        utils::format("/v1/artists/{}/albums", artist_id), {
            { "include_groups", utils::string_join(groups, ",") }
        }
    ));
}

std::vector<track_t> api::get_artist_top_tracks(const item_id_t &artist_id)
{
    return request_item(item_requester<std::vector<track_t>, 1, std::chrono::weeks>(
        utils::format("/v1/artists/{}/top-tracks", artist_id), {}, "tracks"), get_ptr());
}
    
album_t api::get_album(const item_id_t &album_id)
{
    return request_item(item_requester<album_t, 1, std::chrono::weeks>(
        utils::format("/v1/albums/{}", album_id)), get_ptr());
}

std::vector<album_t> api::get_albums(const item_ids_t &ids)
{
    return request_item(several_items_requester<album_t, 1, std::chrono::weeks>(
        "/v1/albums", ids, 20, "albums"), get_ptr());
}

std::vector<track_t> api::get_tracks(const item_ids_t &ids)
{
    return request_item(several_items_requester<track_t, 1, std::chrono::weeks>(
        "/v1/tracks", ids, 20, "tracks"), get_ptr());
}

album_tracks_ptr api::get_album_tracks(const item_id_t &album_id)
{
    return album_tracks_ptr(new album_tracks_t(
        get_ptr(), utils::format("/v1/albums/{}/tracks", album_id)));
}

saved_playlists_ptr api::get_saved_playlists()
{
    return saved_playlists_ptr(new saved_playlists_t(get_ptr(), "/v1/me/playlists"));
}

playlist_t api::get_playlist(const item_id_t &playlist_id)
{
    return request_item(item_requester<playlist_t, 1, std::chrono::days>(
        utils::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist_t::get_fields_filter() },
        }), get_ptr());
}

user_top_tracks_ptr api::get_user_top_tracks()
{
    return user_top_tracks_ptr(new user_top_tracks_t(
        get_ptr(), "/v1/me/top/tracks", {
            { "time_range", "long_term" } // long_term ~1year, medium_term ~6m, short_term - 4w
        }
    ));
}

user_top_artists_ptr api::get_user_top_artists()
{
    return user_top_artists_ptr(new user_top_artists_t(
        get_ptr(), "/v1/me/top/artists", {
            { "time_range", "long_term" } // long_term ~1year, medium_term ~6m, short_term - 4w
        }
    ));
}

saved_tracks_ptr api::get_playlist_tracks(const item_id_t &playlist_id)
{
    return saved_tracks_ptr(new saved_tracks_t(
        get_ptr(),
        utils::format("/v1/playlists/{}/tracks", playlist_id), {
            { "additional_types", "track" },
            { "fields", utils::format("items({}),next,total", saved_track_t::get_fields_filter()) },
        }));
}

playing_queue_t api::get_playing_queue()
{
    return request_item(item_requester<playing_queue_t, 1, std::chrono::seconds>(
        "/v1/me/player/queue"), get_ptr());
}

void api::start_playback(const string &context_uri, const string &track_uri,
                         int position_ms, const item_id_t &device_id)
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
    
    start_playback_base(body.str(), device_id);
}

void api::start_playback(const std::vector<string> &uris, const string &track_uri, const item_id_t &device_id)
{
    assert(uris.size() > 0);

    http::json_body_builder body;

    body.object([&]
    {
        body.insert("uris", uris);
        if (!track_uri.empty())
        {
            body.object("offset", [&]
            {
                body.insert("uri", track_uri);
            });
        }
    });

    start_playback_base(body.str(), device_id);
}

void api::start_playback(const simplified_album_t &album, const simplified_track_t &track)
{
    return start_playback(album.get_uri(), track.get_uri());
}

void api::start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track)
{
    return start_playback(playlist.get_uri(), track.get_uri());
}

void api::resume_playback(const item_id_t &device_id)
{
    return start_playback_base("", device_id);
}

void api::pause_playback(const item_id_t &device_id)
{
    requests_pool.detach_task(
        [&cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {};
            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            if (auto res = put(append_query_params("/v1/me/player/pause", params)); http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([](auto &v) {
                    json::Pointer("/is_playing").Set(v, false);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::skip_to_next(const item_id_t &device_id)
{
    requests_pool.detach_task(
        [this, dev_id = std::as_const(device_id)]
        {
            http::json_body_builder body;

            body.object([&]
            {
                if (!dev_id.empty())
                    body.insert("device_id", dev_id);
            });

            if (auto res = post("/v1/me/player/next", body.str()); !http::is_success(res))
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::skip_to_previous(const item_id_t &device_id)
{
    requests_pool.detach_task(
        [this, dev_id = std::as_const(device_id)]
        {
            http::json_body_builder body;

            body.object([&]
            {
                if (!dev_id.empty())
                    body.insert("device_id", dev_id);
            });
            
            if (auto res = post("/v1/me/player/previous", body.str()); !http::is_success(res))
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::seek_to_position(int position_ms, const item_id_t &device_id)
{
    requests_pool.detach_task(
        [
            position_ms, &cache = *playback,
            dev_id = std::as_const(device_id), this
        ]
        {
            Params params = {
                { "position_ms", std::to_string(position_ms) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            if (auto res = put(append_query_params("/v1/me/player/seek", params)); http::is_success(res))
            {
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([position_ms](auto &v) {
                    json::Pointer("/progress_ms").Set(v, position_ms);
                });
            }
            else
            {
                playback_cmd_error(http::get_status_message(res));
            }
        });
}

void api::toggle_shuffle(bool is_on, const item_id_t &device_id)
{
    requests_pool.detach_task(
        [is_on, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "state", is_on ? "true" : "false" },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });
            
            if (auto res = put(append_query_params("/v1/me/player/shuffle", params)); http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([is_on](auto &v) {
                    json::Pointer("/shuffle_state").Set(v, is_on);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::set_repeat_state(const string &mode, const item_id_t &device_id)
{
    requests_pool.detach_task(
        [mode, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "state", mode },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            if (auto res = put(append_query_params("/v1/me/player/repeat", params)); http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([mode](auto &d) {
                    json::Pointer("/repeat_state").Set(d, mode);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::set_playback_volume(int volume_percent, const item_id_t &device_id)
{
    requests_pool.detach_task(
        [volume_percent, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "volume_percent", std::to_string(volume_percent) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            if (auto res = put(append_query_params("/v1/me/player/volume", params)); http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([volume_percent](auto &d) {
                    json::Pointer("/device/volume_percent").Set(d, volume_percent);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::start_playback_base(const string &body, const item_id_t &device_id)
{
    Params params = {};
    if (!device_id.empty())
        params.insert({ "device_id", device_id });

    requests_pool.detach_task(
        [
            this, &cache = *playback, dev_id = std::as_const(device_id),
            request_url = append_query_params("/v1/me/player/play", params), body
        ]
        {
            auto res = put(request_url, body);
            if (http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([](auto &d) {
                    json::Pointer("/is_playing").Set(d, true);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

wstring api::get_image(const image_t &image, const item_id_t &item_id)
{
    static const wstring cache_folder = utils::format(L"{}\\images", config::get_plugin_data_folder());

    if (!image.is_valid())
    {
        log::global->warn("The given image is invalid, item_id {}", item_id);
        return L"";
    }

    if (!fs::exists(cache_folder))
        fs::create_directories(cache_folder);

    const std::filesystem::path filepath = utils::format(L"{}\\{}.{}.png",
        cache_folder, utils::to_wstring(item_id), image.width);
    
    if (!fs::exists(filepath))
    {
        std::ofstream file(filepath, std::ios_base::binary);
        if (!file.good())
        {
            log::api->error("Cound not create a file for downloading image, {}. {}", utils::to_string(filepath),
                utils::get_last_system_error());
            return L"";
        }

        auto splitted_url = http::split_url(image.url);

        httplib::Client client(splitted_url.first);

        auto res = client.Get(splitted_url.second,
            [&](const char *data, size_t data_length)
            {
                file.write(data, data_length);
                return true;
            });
        
        if (!http::is_success(res))
        {
            log::api->error("An error occured while downloading an image cover for the track: {}, url {}",
                http::get_status_message(res), image.url);
            return L"";
        }
    }
    return filepath;
}

string api::get_lyrics(const track_t &track)
{
    static const wstring cache_folder = utils::format(L"{}\\lyrics", config::get_plugin_data_folder());

    if (!track)
    {
        log::global->warn("Could not retrieve lyrics, the given track is invalid");
        return "";
    }

    if (!fs::exists(cache_folder))
        fs::create_directories(cache_folder);

    const wstring filename = utils::strip_invalid_filename_chars(utils::format(
        L"{} - {} - {} [{}]",
        track.get_artist().name, track.album.name, track.name,
        utils::to_wstring(track.id)));
    
    const std::filesystem::path filepath = utils::format(L"{}\\{}.lrc", cache_folder, filename);
    
    if (fs::exists(filepath))
    {
        if (auto ifs = std::ifstream(filepath))
            return string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    }

    httplib::Client client("https://lrclib.net");

    auto url = httplib::append_query_params("/api/get", {
        { "artist_name", utils::utf8_encode(track.get_artist().name) },
        { "album_name", utils::utf8_encode(track.album.name) },
        { "track_name", utils::utf8_encode(track.name) },
        { "duration", std::to_string(track.duration) },
    });

    if (auto res = client.Get(url); http::is_success(res))
    {
        try
        {
            json::Document doc;
            doc.Parse(res->body);
            
            json::Value lyrics_node;
            
            // first, trying to find synced lyrics if possible
            if (doc.HasMember("syncedLyrics") && !doc["syncedLyrics"].IsNull())
                lyrics_node = doc["syncedLyrics"];
            
            // ... or trying to find just plain lyrics
            else if (doc.HasMember("plainLyrics") && !doc["plainLyrics"].IsNull())
                lyrics_node = doc["plainLyrics"];

            if (!lyrics_node.IsNull())
            {
                auto lyrics = lyrics_node.GetString();

                if (std::ofstream file(filepath); file.good())
                {
                    file << lyrics;
                }
                else
                {
                    log::api->error("Cound not create a file for downloading lyrics, {}. {}", utils::to_string(filepath),
                        utils::get_last_system_error());
                }
                return lyrics;
            }
        }
        catch (const std::exception &ex)
        {
            log::api->error("An error occured while getting lyrics for the track {}, {}", track.id, ex.what());
        }
    }
    else
    {
        log::api->error("An error occured while downloading the lyrics for the track {}: {}, url {}",
            track.id, http::get_status_message(res), url);
    }
    return "";
}

bool api::is_request_cached(const string &url) const
{
    string u = http::trim_domain(url);
    return api_responses_cache->is_cached(u) && api_responses_cache->get(u).is_valid();
}

bool api::is_endpoint_rate_limited(const string &endpoint_name) const
{
    return guards.contains(endpoint_name) && guards.at(endpoint_name).is_rate_limited();
}

void api::cancel_pending_requests(bool wait_for_result)
{
    cancel_flag = true;

    // notifying all the threads to checks their statuses
    for (auto &guard: guards)
        guard.second.notify_all();
    
    log::api->debug("Cancelling pending API requests, wait for result: {}", wait_for_result);

    // a very unreliable piece of code, waiting for 1.5s for all the pending threads to
    // wake up, check their statuses and drop unfinished requests; in case of plugin closure,
    // we do not wait for the result to avoid Far freezing
    if (wait_for_result)
    {
        auto r = std::async(std::launch::async, [this] {
            std::this_thread::sleep_for(1.5s);
            cancel_flag = false;
        });
    }
}

httplib::Result api::get(const string &request_url, clock_t::duration cache_for, bool retry_429)
{
    string cached_etag = "";
    string url = http::trim_domain(request_url);

    if (api_responses_cache->is_cached(url))
    {
        // we have a cache for the requested url and it is still valid
        if (const auto &cache = api_responses_cache->get(url))
        {
            Result res(std::make_unique<Response>(), Error::Success);
            res->status = OK_200;
            res->body = cache.body;
            return res;
        }
        // ..or it is invalid already, so we use its ETag to refresh the response
        else
        {
            cached_etag = cache.etag;
        }
    }

    Result res(std::make_unique<Response>(), Error::Success);

    while (1)
    {
        // the target endpoint is rate limited...
        if (auto &ep = get_endpoint(url); ep.is_rate_limited())
        {
            // in case the request specifies `retry` flag, we lock the thread and wait for
            // the endpoint to get released
            if (retry_429)
            {
                ep.wait([this] { return cancel_flag; });
            }
            // ..otherwise just return the error http request result
            else
            {
                return get_rate_limited_response(ep.get_expires_at());
            }
        }

        // if we get here with the `cancel_flag` is `true`, this means that user requested to
        // cancel all the current pending requests via Escape on Waiting splash dialog, all the
        // pending thread woke up and reached this codepoint with the activated flag
        if (cancel_flag)
        {
            log::api->warn("The request is cancelled by user: {}", url);
            res->status = http::CancelledByUser_470;
            return res;
        }

        // pure debugging code for emulating randomly 429 http error
        // if (d(gen))
        // {
        //     // in some rare cases `res` could not contain any response object, just finish execution in such case
        //     if (res = get_client()->Get(url, {{ "If-None-Match", cached_etag }}); !res)
        //         return res;
        // }
        // else
        // {
        //     res->status = TooManyRequests_429;
        //     res->set_header("retry-after", "15");
        // }
        
        // in some rare cases `res` could not contain any response object, just finish execution in such case
        if (res = get_client()->Get(url, {{ "If-None-Match", cached_etag }}); !res)
            return res;

        // in case we were rate-limited, we retry the request a bit postponed
        if (res->status == TooManyRequests_429)
        {
            auto retry_after = 3s; // default retry delay in case it is not present in the headers
            
            if (res->has_header("retry-after"))
                // +1s just in case
                retry_after = std::chrono::seconds(std::stoi(res->get_header_value("retry-after", 0))) + 1s;

            // in any case we update the endpoint's expiration time, so all the subsequent requests
            // will not bother API server
            auto &ep = get_endpoint(url);
            ep.set_expires_at(utils::clock_t::now() + retry_after);

            log::api->warn("The endpoint \"{}\" is rate limited for {}", ep.get_name(), utils::format("{:%T}", retry_after));

            if (retry_429)
            {
                log::api->warn("The request is put into the queue for retry in {} seconds: {}", retry_after.count(), url);
                continue;
            }
        }

        // ...in all other cases - just process and return the result
        break;
    }

    if (res->status == OK_200)
    {
        auto etag = res->get_header_value("etag", "");

        // caching only requests which have ETag or `cache-for` instruction
        if (!etag.empty() || cache_for > clock_t::duration::zero())
            api_responses_cache->store(url, res->body, etag, cache_for);
    }
    else if (res->status == NotModified_304)
    {
        const auto &cache = api_responses_cache->get(url);

        // replacing empty body with the cached one, so the client
        // does not see the difference
        res->body = cache.body;

        // the response is still valid, so caching for a session or any other
        // time if needed
        if (cache_for != clock_t::duration::zero())
            api_responses_cache->store(url, cache.body, cache.etag, cache_for);
    }
    return res;
}

httplib::Result api::put(const string &request_url, const string &body)
{
    // checking if the target endpoint is available
    if (auto &ep = get_endpoint(request_url); ep.is_rate_limited())
        return get_rate_limited_response(ep.get_expires_at());

    httplib::Result res;
    auto client = get_client();

    if (!body.empty())
        res = client->Put(request_url, body, "application/json");
    else
        res = client->Put(request_url);
    
    if (http::is_success(res))
    {
        // invalidate all cached GET responses with the same base url
        api_responses_cache->invalidate(request_url);
    }

    return res;
}

httplib::Result api::del(const string &request_url, const string &body)
{
    // checking if the target endpoint is available
    if (auto &ep = get_endpoint(request_url); ep.is_rate_limited())
        return get_rate_limited_response(ep.get_expires_at());
    
    auto res = get_client()->Delete(request_url, body, "application/json");
    
    if (http::is_success(res))
    {
        // invalidate all the cached repsonses with the same base urls
        api_responses_cache->invalidate(request_url);
    }

    return res;
}

httplib::Result api::post(const string &request_url, const string &body)
{
    // checking if the target endpoint is available
    if (auto &ep = get_endpoint(request_url); ep.is_rate_limited())
        return get_rate_limited_response(ep.get_expires_at());
    
    auto res = get_client()->Post(request_url, body, "application/json");
    
    if (http::is_success(res))
    {
        // invalidate all the cached repsonses with the same base urls
        api_responses_cache->invalidate(request_url);
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

} // namespace spotify
} // namespace spotifar