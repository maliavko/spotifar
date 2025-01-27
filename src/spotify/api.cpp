#include "api.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;
        using json = nlohmann::json;
        using namespace httplib;

        bool is_success(int status)
        {
            return status == OK_200 || status == NoContent_204 || status == NotModified_304;
        }

        // TODO: reconsider these functions
        std::string dump_headers(const Headers &headers) {
            std::string s;
            char buf[BUFSIZ];

            for (auto it = headers.begin(); it != headers.end(); ++it) {
                const auto &x = *it;
                snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
                s += buf;
            }

            return s;
        }

        std::string dump_http_error(const Request &req, const Response &res)
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

        Api::Api():
            client(SPOTIFY_API_URL),
            logger(spdlog::get(utils::LOGGER_API)),
            pool(4)
        {
            static const std::set<std::string> exclude{
                "/v1/me/player",
                "/v1/me/player/devices",
            };

            client.set_logger(
                [this](const Request &req, const Response &res)
                {
                    if (is_success(res.status))
                    {
                        if (!exclude.contains(req.path))
                            logger->debug("A successful HTTP request has been performed (code={}): [{}] {}",
                                          res.status, req.method, req.path);
                    }
                    else
                        logger->error(dump_http_error(req, res));
                });
            
            client.set_default_headers(Headers{
                {"Content-Type", "application/json; charset=utf-8"},
            });

            auth = std::make_unique<AuthCache>(
                this, config::get_client_id(), config::get_client_secret(),
                config::get_localhost_port());
            devices = std::make_unique<DevicesCache>(this);
            // history = std::make_unique<PlayedHistory>(this);
            playback = std::make_unique<PlaybackCache>(this);
            library = std::make_unique<LibraryCache>(this);

            caches.assign({
                auth.get(), playback.get(), devices.get(), library.get()//, history.get(),
            });
        }

        Api::~Api()
        {
            caches.clear();
            logger = nullptr;
        }

        bool Api::init()
        {
            auto ctx = config::lock_settings();
            for (auto &c: caches)
                c->read(*ctx);

            auto s = ctx->get_str(L"requests", "");
            if (!s.empty())
                requests_cache = json::parse(s);

            launch_sync_worker();

            return true;
        }
        
        void Api::shutdown()
        {
            auto ctx = config::lock_settings();
            for (auto &c: caches)
                c->write(*ctx);

            ctx->set_str(L"requests", requests_cache.dump());
            
            shutdown_sync_worker();

            ObserverManager::clear<ApiObserver>();
        }
        
        void Api::start_playback(const string &context_uri, const string &track_uri,
                                 int position_ms, const string &device_id)
        {
            Params params = {};
            if (!device_id.empty())
                params.insert({ "device_id", device_id });

            string body = ""; 
            if (!context_uri.empty())
            {
                json o{
                    { "context_uri", context_uri },
                    { "position_ms", position_ms },
                };

                if (!track_uri.empty())
                    o["offset"] = { { "uri", track_uri }, };
                
                body = o.dump();
            }

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
                        res = c.Put(request_url, body, "application/json");

                    if (is_success(res))
                        cache.patch({ { "is_playing", true } });
                });
        }
        
        void Api::start_playback(const SimplifiedAlbum &album, const SimplifiedTrack &track)
        {
            return start_playback(album.get_uri(), track.get_uri());
        }
        
        void Api::start_playback(const SimplifiedPlaylist &playlist, const SimplifiedTrack &track)
        {
            return start_playback(playlist.get_uri(), track.get_uri());
        }

        void Api::pause_playback(const string &device_id)
        {
            pool.detach_task(
                [&c = this->client, &cache = *playback, dev_id = std::as_const(device_id)]
                {
                    Params params = {};
                    if (!dev_id.empty())
                        params.insert({ "device_id", dev_id });

                    auto r = c.Put(append_query_params("/v1/me/player/pause", params));
                    if (r->status == httplib::OK_200)
                        cache.patch({ { "is_playing", false } });
                });
        }
        
        void Api::skip_to_next(const string &device_id)
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

        void Api::skip_to_previous(const string &device_id)
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
        
        void Api::seek_to_position(int position_ms, const string &device_id)
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
                        cache.patch({ { "progress_ms", position_ms } });
                });
        }

        void Api::toggle_shuffle(bool is_on, const string &device_id)
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

        void Api::set_repeat_state(const std::string &mode, const string &device_id)
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

        void Api::set_playback_volume(int volume_percent, const string &device_id)
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
        
        void Api::transfer_playback(const std::string &device_id, bool start_playing)
        {   
            auto devices = get_available_devices();
            auto device_it = std::find_if(devices.begin(), devices.end(),
                [&device_id](auto &d) { return d.id == device_id; });

            if (device_it == devices.end())
                return logger->error("There is no devices with the given id={}", device_id);

            if (device_it->is_active)
                return logger->warn("The given device is already active {}", device_it->to_str());
            
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
        
        AlbumsCollection Api::get_albums(const std::string &artist_id)
        {
            AlbumsCollection albums;

            Params params = {
                { "limit", "50" },
                { "offset", "0" },
                { "include_groups", "album" }
            };

            std::string request_url = append_query_params(
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
        
        PlaylistsCollection Api::get_playlists()
        {
            PlaylistsCollection playlists;

            Params params = {
                { "limit", "50" },
                { "offset", "0" },
            };

            std::string request_url = append_query_params("/v1/me/playlists", params);

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
        
        std::map<string, SimplifiedTrack> Api::get_tracks(const std::string &album_id)
        {
            std::map<string, SimplifiedTrack> tracks;

            Params params = {
                { "limit", "50" },
                { "offset", "0" }
            };

            std::string request_url = append_query_params(
                std::format("/v1/albums/{}/tracks", album_id), params);

            do
            {
                //auto r = client.Get(request_url);
                auto r = get(request_url);

                json data = json::parse(r->body);
                for (json& tj : data["items"])
                {
                    auto t = tj.get<SimplifiedTrack>();
                    tracks[t.id] = t;
                }

                json next = data["next"];
                if (next.is_null())
                    break;
                
                next.get_to(request_url);
            } while (1);

            return tracks;
        }

        void Api::launch_sync_worker()
        {
            std::packaged_task<void()> task([this]
            {
                // clock::time_point now;
                std::string exit_msg = "";
                const std::lock_guard worker_lock(sync_worker_mutex);

                try
                {
                    while (is_worker_listening)
                    {
                        pool.detach_loop(0ULL, caches.size(),
                            [&caches = this->caches](const std::size_t idx) {
                                caches[idx]->resync();
                            }, BS::pr::high);
                        pool.wait();
                        
                        // notify listeners that they can perform some aux operations in a
                        // background thread
                        ObserverManager::notify(&BasicApiObserver::on_sync_thread_tick);

                        std::this_thread::sleep_for(50ms);
                    }
                }
                catch (const std::exception &ex)
                {
                    // TODO: what if there is an error, but no playback is opened
                    exit_msg = ex.what();
                    logger->critical("An exception occured while syncing up with an API: {}", exit_msg);
                }
                
                ObserverManager::notify(&BasicApiObserver::on_playback_sync_finished, exit_msg);
            });

            is_worker_listening = true;
            std::thread(std::move(task)).detach();
            logger->info("An API sync worker has been launched");
        }

        void Api::shutdown_sync_worker()
        {
            is_worker_listening = false;
            
            // trying to acquare a sync worker mutex, giving it time to clean up
            // all the resources
            const std::lock_guard worker_lock(sync_worker_mutex);
            logger->info("An API sync worker has been stopped");
        }
        
        httplib::Result Api::get(const string &request_url)
        {
            auto cache_it = requests_cache.find(request_url);

            string cached_etag = "";
            if (cache_it != requests_cache.end())
                cached_etag = cache_it->at("etag");

            if (auto r = client.Get(request_url, {{ "If-None-Match", cached_etag }}))
            {
                if (r->status == httplib::OK_200)
                {
                    auto etag = r->get_header_value("etag", "");
                    if (!etag.empty())
                    {
                        requests_cache[request_url] = {
                            { "body", r->body },
                            { "etag", etag },
                        };
                    }

                    auto cache_control = r->get_header_value("cache-control", "");
                    if (!cache_control.empty() && !cache_control.ends_with("max-age=0"))
                    {
                        spdlog::debug("Cache control found in the response {}", cache_control);
                    }

                    return r;
                }
                else if (r->status == httplib::NotModified_304)
                {
                    httplib::Result result(std::make_unique<httplib::Response>(), httplib::Error::Success);
                    result.value().status = httplib::OK_200;
                    result.value().body = cache_it->at("body");

                    return result;
                }
            }
            return httplib::Result();
        }
    }
}