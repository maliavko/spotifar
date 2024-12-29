#include "api.hpp"
#include "config.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;
        using json = nlohmann::json;
        using namespace httplib;

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
            endpoint(SPOTIFY_API_URL),
            is_worker_listening(false),
            logger(spdlog::get(utils::LOGGER_API)),
            playback_observers(0)
        {
            static const std::set<std::string> exclude{
                "/v1/me/player",
                "/v1/me/player/devices",
            };

            endpoint.set_logger([this](const Request &req, const Response &res)
            {
                if (res.status != OK_200 || res.status != NoContent_204)
                {
                    if (!exclude.contains(req.path))
                        logger->debug("A successful HTTP request has been performed: [{}] {}", req.method, req.path);
                }
                else
                    logger->error(dump_http_error(req, res));
            });
            
            endpoint.set_default_headers(Headers{
                {"Content-Type", "application/json; charset=utf-8"},
            });

            auth = std::make_unique<AuthCache>(
                &endpoint, config::get_client_id(), config::get_client_secret(),
                config::get_localhost_port());
            devices = std::make_unique<DevicesCache>(&endpoint);
            history = std::make_unique<PlayedHistory>(&endpoint);
            playback = std::make_unique<PlaybackCache>(&endpoint);

            caches.assign({
                auth.get(), history.get(), playback.get(), devices.get()
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

            launch_sync_worker();

            return true;
        }
        
        void Api::shutdown()
        {
            auto ctx = config::lock_settings();
            for (auto &c: caches)
                c->write(*ctx);
            
            shutdown_sync_worker();

            ObserverManager::clear<ApiObserver>();
        }
        
        void Api::start_playback(const string &context_uri, const string &track_uri,
                                 int position_ms, const string &device_id)
        {
            Params params = {};
            Result res;

            if (!device_id.empty())
                params.insert({ "device_id", device_id });

            auto request_url = append_query_params("/v1/me/player/play", params);

            if (!context_uri.empty())
            {
                // TODO: not tested
                json o{
                    { "context_uri", context_uri },
                    { "position_ms", position_ms },
                };

                if (!track_uri.empty())
                    o["offset"] = { { "uri", track_uri }, };
                
                res = endpoint.Put(request_url, o.dump(), "application/json");
            }
            else
            {
                res = endpoint.Put(request_url);
            }
            
            if (res->status == OK_200 || res->status == NoContent_204)
                get_playback_cache().patch_data({ { "is_playing", true } });
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
            Params params = {};

            if (!device_id.empty())
                params.insert({ "device_id", device_id });

            auto r = endpoint.Put(append_query_params("/v1/me/player/pause", params));
            if (r->status == httplib::OK_200)
                get_playback_cache().patch_data({ { "is_playing", false } });
        }
        
        void Api::skip_to_next()
        {
            // TODO: unfinished
            auto r = endpoint.Post("/v1/me/player/next");
        }

        void Api::skip_to_previous()
        {
            // TODO: unfinished
            auto r = endpoint.Post("/v1/me/player/previous");
        }
        
        void Api::seek_to_position(int position_ms, const string &device_id)
        {
            // TODO: unfinished
            Params params = {
                { "position_ms", std::to_string(position_ms) },
            };

            if (!device_id.empty())
                params.insert({ "device_id", device_id });

            auto res = endpoint.Put(append_query_params("/v1/me/player/seek", params));

            if (res->status == OK_200 || res->status == NoContent_204)
                get_playback_cache().patch_data({ { "progress_ms", position_ms } });
        }

        void Api::toggle_shuffle(bool is_on, const string &device_id)
        {
            Params params = {
                { "state", is_on ? "true" : "false" },
            };

            if (!device_id.empty())
                params.insert({ "device_id", device_id });

            auto res = endpoint.Put(append_query_params("/v1/me/player/shuffle", params));
            if (res->status == OK_200 || res->status == NoContent_204)
                get_playback_cache().patch_data({
                    { "shuffle_state", is_on }
                });
        }

        void Api::set_repeat_state(const std::string &mode, const string &device_id)
        {
            Params params = {
                { "state", mode },
            };

            if (!device_id.empty())
                params.insert({ "device_id", device_id });

            auto res = endpoint.Put(append_query_params("/v1/me/player/repeat", params));
            if (res->status == OK_200 || res->status == NoContent_204)
                get_playback_cache().patch_data({
                    { "repeat_state", mode }
                });
        }

        void Api::set_playback_volume(int volume_percent)
        {
            auto res = endpoint.Put(append_query_params("/v1/me/player/volume",
                Params{{ "volume_percent", std::to_string(volume_percent) }}));

            if (res->status == OK_200 || res->status == NoContent_204)
                get_playback_cache().patch_data({
                    { "device", {
                        { "volume_percent", volume_percent }
                    } }
                });
        }
        
        bool Api::transfer_playback(const std::string &device_id, bool start_playing)
        {   
            auto devices = get_available_devices();
            auto device_it = std::find_if(devices.begin(), devices.end(),
                [&device_id](auto &d) { return d.id == device_id; });

            if (device_it == devices.end())
            {
                logger->error("There is no devices with the given id={}", device_id);
                return false;
            }

            if (device_it->is_active)
            {
                logger->warn("The given device is already active {}", device_it->to_str());
                return true;
            }

            json body{
                { "device_ids", { device_id } },
                { "play", start_playing },
            };

            logger->info("Transferring playback to the device {}, autoplay {}",
                device_it->to_str(), start_playing);

            auto res = endpoint.Put("/v1/me/player", body.dump(), "application/json");
            return res->status == NoContent_204;
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
                auto r = endpoint.Get(request_url);

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
                auto r = endpoint.Get(request_url);

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
                auto r = endpoint.Get(request_url);

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
        
        ArtistsCollection Api::get_artists()
        {
            json after = "";
            ArtistsCollection artists;

            do
            {
                Params params = {
                    { "type", "artist" },
                    { "limit", "50" },
                    { "after", after.get<std::string>() },
                };

                auto r = endpoint.Get("/v1/me/following", params, Headers());
                json data = json::parse(r->body)["artists"];

                for (json& aj : data["items"])
                {
                    auto a = aj.get<Artist>();
                    artists[a.id] = a;
                }
                after = data["cursors"]["after"];
            }
            while (!after.is_null());

            return artists;
        }

        void Api::launch_sync_worker()
        {
            std::packaged_task<void()> task([this]
            {
                clock::time_point now;
                std::string exit_msg = "";
                const std::lock_guard<std::mutex> worker_lock(sync_worker_mutex);

                try
                {
                    while (is_worker_listening)
                    {
                        now = clock::now();

                        // TODO: errors in this function raises on_playback_sync_finished, which is not valid
                        // for this situation, come up with some different errors handling
                        // NOTE: very experimental, if any cache is invalidated in this tick, the others are skipped.
                        // in theory ~20Hz rate should supply all the consumers with enough frames to sync-up
                        for (auto &c: caches)
                            if (c->resync())
                                continue;
                        
                        ObserverManager::notify(&BasicApiObserver::on_sync_thread_tick);

                        #ifdef _DEBUG
                        if (clock::now() - now > 500ms)
                            logger->warn(std::format("Sync thread tick overspend, {:%T}ms",
                                clock::now() - now));
                        #endif

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
            const std::lock_guard<std::mutex> worker_lock(sync_worker_mutex);
            logger->info("An API sync worker has been stopped");
        }
    }
}