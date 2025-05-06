#include "api.hpp"

namespace spotifar { namespace spotify {

using namespace httplib;
using namespace utils;

const string spotify_api_url = "https://api.spotify.com";

// majority of time the threads are not needed, but in case of requesting
// a collection with bunch of pages we perform them asynchronously
const size_t pool_size = 12;

// a helper-function to avoid copy-pasting. Performs an execution of a given
// requester, checks the result and returns it back
template<class R>
auto request_item(R &&requester, api_proxy_ptr api) -> typename R::result_t
{
    if (requester.execute(api))
        return requester.get();
    
    // perhaps at some point it'd be good to add an error propagation here
    // to show to the user
    return {};
}

// a helpers function to dispatch a playback command execution error higher
// to the listeners
template <typename... Args>
void playback_cmd_error(string msg_fmt, Args &&...args)
{
    auto formatted = std::vformat(msg_fmt, std::make_format_args(args...));
    
    log::api->error(formatted);

    utils::far3::synchro_tasks::dispatch_event(
        &api_requests_observer::on_playback_command_failed, formatted);
}


using releases_t = std::vector<simplified_album_t>;

class recent_releases: public json_cache<releases_t>
{
public:
    recent_releases(api_interface *api): json_cache<releases_t>(L"RecentReleases"), api_proxy(api), pool(1)
    {
        artists = api_proxy->get_followed_artists();
    }

    ~recent_releases() { api_proxy = nullptr; pool.purge(); }
protected:
    bool is_active() const override
    {
        return api_proxy->is_authenticated();
    }

    bool request_data(releases_t &data) override
    {
        if (is_in_sync)
        {
            if (pool.get_tasks_total() == 0)
            {
                data = interim_data;
                is_in_sync = false;
            }
            
            return !is_in_sync; // `true` means a succesful end of sync
        }

        interim_data.clear();

        if (artists->fetch(false, false))
        {
            auto time_treshold = utils::clock_t::now() - std::chrono::weeks{2};
            artists->resize(15); // TODO: remove
            
            pool.detach_sequence(0ULL, artists->size(),
                [this, time_treshold](const std::size_t idx)
                {
                    const auto &artist = (*artists)[idx];
                    auto albums = api_proxy->get_artist_albums(artist.id);
                    bool is_cached = albums->is_cached();
                    if (albums->fetch(false, false))
                    {
                        log::api->debug("Searching for new releases, {} [{}]", utils::to_string(artist.name), artist.id);
                        {
                            std::lock_guard lock(data_access);

                            for (const auto &album: *albums)
                                if (album.get_release_date() > time_treshold)
                                    interim_data.push_back(album);
                        }
                    }

                    if (!is_cached)
                        std::this_thread::sleep_for(
                            utils::events::has_observers<playback_observer>() ? 15s : 5s);
                });
            
            is_in_sync = true;
        }

        return false;
    }

    auto get_sync_interval() const -> clock_t::duration override
    {
        return 60s;
    }

    void on_data_synced(const releases_t &data, const releases_t &prev_data) override
    {
        log::global->debug("11111111111 new releases found:");

        for (const auto &album: data)
            log::global->debug("{} - {}", utils::to_string(album.artists[0].name), utils::to_string(album.name));
    }
private:
    api_interface *api_proxy;
    BS::thread_pool pool;
    
    std::mutex data_access;
    data_t interim_data{};

    bool is_in_sync = false;
    followed_artists_ptr artists;
};

//----------------------------------------------------------------------------------------------
api::api(): pool(pool_size)
{
}

api::~api()
{
    caches.clear();
}

bool api::start()
{
    auth = std::make_unique<auth_cache>(this, config::get_client_id(), config::get_client_secret(), config::get_localhost_port());
    devices = std::make_unique<devices_cache>(this);
    history = std::make_unique<play_history>(this);
    playback = std::make_unique<playback_cache>(this);

    static auto rr = std::make_unique<recent_releases>(this);

    caches.assign({ auth.get(), /*playback.get(), devices.get(),*/ history.get(), rr.get() });

    auto ctx = config::lock_settings();

    // initializing persistent caches
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->read(*ctx); });

    // initializing http responses cache
    api_responses_cache.start();

    return true;
}

void api::shutdown()
{
    auto ctx = config::lock_settings();
    std::for_each(caches.begin(), caches.end(), [ctx](auto &c) { c->shutdown(*ctx); });

    pool.purge(); // remove unfinished tasks from the queue

    api_responses_cache.shutdown();
}

void api::tick()
{
    auto future = pool.submit_loop(0ULL, caches.size(),
        [&caches = this->caches](const std::size_t idx) {
            caches[idx]->resync();
        }, BS::pr::high);
    future.get();
}

const playback_state_t& api::get_playback_state(bool force_resync)
{
    if (force_resync)
        playback->resync(true);
    
    return playback->get();
}

const devices_t& api::get_available_devices(bool force_resync)
{
    if (force_resync)
        devices->resync(true);
    
    return devices->get();
}

const history_items_t& api::get_play_history(bool force_resync)
{
    if (force_resync)
        history->resync(true);
    
    return history->get();
}

artist_t api::get_artist(const item_id_t &artist_id)
{
    return request_item(item_requester<artist_t, 1, std::chrono::days>(
        std::format("/v1/artists/{}", artist_id)), get_ptr());
}

std::vector<artist_t> api::get_artists(const item_ids_t &ids)
{
    return request_item(several_items_requester<artist_t>(
        "/v1/artists", ids, 50, "artists"), get_ptr());
}

followed_artists_ptr api::get_followed_artists()
{
    return followed_artists_ptr(new followed_artists_t(
        get_ptr(),
        "/v1/me/following", {
            { "type", "artist" }
        },
        "artists"
    ));
}

artist_albums_ptr api::get_artist_albums(const item_id_t &artist_id)
{
    return artist_albums_ptr(new artist_albums_t(
        get_ptr(),
        std::format("/v1/artists/{}/albums", artist_id), {
            { "include_groups", "album,single" }
        }
    ));
}

saved_albums_ptr api::get_saved_albums()
{
    return saved_albums_ptr(new saved_albums_t(get_ptr(), "/v1/me/albums"));
}

new_releases_ptr api::get_new_releases()
{
    return new_releases_ptr(new new_releases_t(
        get_ptr(), "/v1/browse/new-releases", {}, "albums"));
}

std::vector<track_t> api::get_artist_top_tracks(const item_id_t &artist_id)
{
    return request_item(item_requester<std::vector<track_t>, 1, std::chrono::days>(
        std::format("/v1/artists/{}/top-tracks", artist_id), {}, "tracks"), get_ptr());
}
    
album_t api::get_album(const item_id_t &album_id)
{
    return request_item(item_requester<album_t, 1, std::chrono::days>(
        std::format("/v1/albums/{}", album_id)), get_ptr());
}

std::vector<album_t> api::get_albums(const item_ids_t &ids)
{
    return request_item(several_items_requester<album_t>(
        "/v1/albums", ids, 20, "albums"), get_ptr());
}

album_tracks_ptr api::get_album_tracks(const item_id_t &album_id)
{
    return album_tracks_ptr(new album_tracks_t(
        get_ptr(), std::format("/v1/albums/{}/tracks", album_id)));
}

saved_tracks_ptr api::get_saved_tracks()
{
    return saved_tracks_ptr(new saved_tracks_t(get_ptr(), "/v1/me/tracks"));
}

saved_playlists_ptr api::get_saved_playlists()
{
    return saved_playlists_ptr(new saved_playlists_t(get_ptr(), "/v1/me/playlists"));
}

playlist_t api::get_playlist(const item_id_t &playlist_id)
{
    return request_item(item_requester<playlist_t, 1, std::chrono::days>(
        std::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist_t::get_fields_filter() },
        }), get_ptr());
}

saved_tracks_ptr api::get_playlist_tracks(const item_id_t &playlist_id)
{
    return saved_tracks_ptr(new saved_tracks_t(
        get_ptr(),
        std::format("/v1/playlists/{}/tracks", playlist_id), {
            { "additional_types", "track" },
            { "fields", std::format("items({}),next,total", saved_track_t::get_fields_filter()) },
        }));
}

playing_queue_t api::get_playing_queue()
{
    return request_item(item_requester<playing_queue_t, 1, std::chrono::days>("/v1/me/player/queue"), get_ptr());
}

bool api::check_saved_track(const item_id_t &track_id)
{
    const auto &flags = check_saved_tracks({ track_id });
    return flags.size() > 0 && flags[0];
}

std::deque<bool> api::check_saved_tracks(const item_ids_t &ids)
{
    // bloody hell, damn vector specialization with bools is not a real vector,
    // it cannot return ref to bools, so deque is used instead
    return request_item(several_items_requester<bool, std::deque<bool>>(
        "/v1/me/tracks/contains", ids, 50), get_ptr());
}

bool api::save_tracks(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto res = put("/v1/me/tracks", body.str());
    if (!http::is_success(res))
    {
        playback_cmd_error(http::get_status_message(res));
        return false;
    }
    return true;
}

bool api::remove_saved_tracks(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto res = del("/v1/me/tracks", body.str());
    if (!http::is_success(res))
    {
        playback_cmd_error(http::get_status_message(res));
        return false;
    }

    return true;
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
    
    start_playback_raw(body.str(), device_id);
}

void api::start_playback(const std::vector<string> &uris, const item_id_t &device_id)
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

void api::resume_playback(const item_id_t &device_id)
{
    return start_playback_raw("", device_id);
}

void api::toggle_playback(const item_id_t &device_id)
{
    playback->resync(true);
    const auto &state = playback->get();
    if (!state.is_playing)
        return resume_playback(device_id);
    else
        return pause_playback(device_id);
}

void api::pause_playback(const item_id_t &device_id)
{
    pool.detach_task(
        [&cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {};
            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/pause", params));
            if (http::is_success(res))
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
    pool.detach_task(
        [this, dev_id = std::as_const(device_id)]
        {
            http::json_body_builder body;

            body.object([&]
            {
                if (!dev_id.empty())
                    body.insert("device_id", dev_id);
            });

            auto res = post("/v1/me/player/next", body.str());
            if (!http::is_success(res))
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::skip_to_previous(const item_id_t &device_id)
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
            
            auto res = post("/v1/me/player/previous", body.str());
            if (!http::is_success(res))
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::seek_to_position(int position_ms, const item_id_t &device_id)
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
            if (http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([position_ms](auto &v) {
                    json::Pointer("/progress_ms").Set(v, position_ms);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::toggle_shuffle(bool is_on, const item_id_t &device_id)
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
            if (http::is_success(res))
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
    pool.detach_task(
        [mode, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "state", mode },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/repeat", params));
            if (http::is_success(res))
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
    pool.detach_task(
        [volume_percent, &cache = *playback, dev_id = std::as_const(device_id), this]
        {
            Params params = {
                { "volume_percent", std::to_string(volume_percent) },
            };

            if (!dev_id.empty())
                params.insert({ "device_id", dev_id });

            auto res = put(append_query_params("/v1/me/player/volume", params));
            if (http::is_success(res))
                // patching the data in the cache, so the client represents a correct UI,
                // while the updates still coming from the server
                cache.patch([volume_percent](auto &d) {
                    json::Pointer("/device/volume_percent").Set(d, volume_percent);
                });
            else
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::transfer_playback(const item_id_t &device_id, bool start_playing)
{
    const auto &devices = get_available_devices();
    auto device_it = std::find_if(devices.begin(), devices.end(),
        [&device_id](const auto &d) { return d.id == device_id; });

    if (device_it == devices.end())
        return playback_cmd_error("There is no devices with the given id={}", device_id);

    if (device_it->is_active)
        return playback_cmd_error("The given device is already active, {}", device_it->to_str());
    
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
            if (!http::is_success(res))
                playback_cmd_error(http::get_status_message(res));
        });
}

void api::start_playback_raw(const string &body, const item_id_t &device_id)
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
    static const wstring cache_folder = std::format(L"{}\\images", config::get_plugin_data_folder());

    if (!std::filesystem::exists(cache_folder))
        std::filesystem::create_directories(cache_folder);

    const wstring filepath = std::format(L"{}\\{}.{}.png", cache_folder, utils::to_wstring(item_id), image.width);
    
    if (!std::filesystem::exists(filepath))
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

bool api::is_request_cached(const string &url) const
{
    string u = http::trim_domain(url);
    return api_responses_cache.is_cached(u) && api_responses_cache.get(u).is_valid();
}

httplib::Result api::get(const string &request_url, clock_t::duration cache_for)
{
    string cached_etag = "";
    string url = http::trim_domain(request_url);

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

    auto res = get_client()->Get(request_url, {{ "If-None-Match", cached_etag }});
    if (http::is_success(res))
    {
        if (res->status == OK_200)
        {
            auto etag = res->get_header_value("etag", "");

            // caching only requests which have ETag or `cache-for` instruction
            if (!etag.empty() || cache_for > clock_t::duration::zero())
                api_responses_cache.store(url, res->body, etag, cache_for);
        }
        else if (res->status == NotModified_304)
        {
            const auto &cache = api_responses_cache.get(url);

            // replacing empty body with the cached one, so the client
            // does not see the difference
            res->body = cache.body;

            // the response is still valid, so caching for a session or any other
            // time if needed
            if (cache_for != clock_t::duration::zero())
                api_responses_cache.store(url, cache.body, cache.etag, cache_for);
        }
    }
    else
    {
        auto retry_after = std::stoi(res->get_header_value("retry-after", 0));

        if (res->status == TooManyRequests_429)
            throw std::runtime_error(std::format("The app has been rate limited, retry after {:%T}",
                std::chrono::seconds(retry_after)));
    }

    return res;
}

httplib::Result api::put(const string &request_url, const string &body)
{
    httplib::Result res;
    auto client = get_client();

    if (!body.empty())
        res = client->Put(request_url, body, "application/json");
    else
        res = client->Put(request_url);
    
    if (http::is_success(res))
    {
        // invalidate all cached GET responses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

httplib::Result api::del(const string &request_url, const string &body)
{
    auto res = get_client()->Delete(request_url, body, "application/json");
    
    if (http::is_success(res))
    {
        // invalidate all the cached repsonses with the same base urls
        api_responses_cache.invalidate(request_url);
    }

    return res;
}

httplib::Result api::post(const string &request_url, const string &body)
{
    auto res = get_client()->Post(request_url, body, "application/json");
    
    if (http::is_success(res))
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

} // namespace spotify
} // namespace spotifar