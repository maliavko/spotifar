#include "api.hpp"
#include "auth.hpp"
#include "playback.hpp"
#include "history.hpp"
#include "devices.hpp"
#include "config.hpp"

namespace spotifar
{
    namespace spotify
    {
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

        const string Api::SPOTIFY_API_URL = "https://api.spotify.com";

        Api::Api(const string &client_id, const string &client_secret, int port):
            endpoint(std::make_unique<Client>(SPOTIFY_API_URL)),
            port(port),
            client_id(client_id),
            client_secret(client_secret),
            is_worker_listening(false),
            logger(spdlog::get(utils::LOGGER_API)),
            playback_observers(0)
        {
            endpoint->set_logger([this](const Request &req, const Response &res)
            {
                // TODO: add one-line debug logging for all requests done
                if (res.status != OK_200 && res.status != NoContent_204)
                    logger->error(dump_http_error(req, res));
            });
            
            endpoint->set_default_headers(Headers{
                {"Content-Type", "application/json; charset=utf-8"},
            });

            // NOTE: the order should match declared indicies enum CacheIdx
            cache.push_back(std::make_unique<AuthCache>(
                endpoint.get(), client_id, client_secret, port
                ));
            cache.push_back(std::make_unique<PlayedHistory>(endpoint.get()));
            cache.push_back(std::make_unique<PlaybackCache>(endpoint.get()));
            cache.push_back(std::make_unique<DevicesCache>(endpoint.get()));
        }

        Api::~Api()
        {
            cache.clear();

            logger = nullptr;
            endpoint = nullptr;
        }

        bool Api::init()
        {
            auto ctx = config::lock_settings();
            for (auto &c: cache)
                c->read(*ctx);

            launch_sync_worker();

            return true;
        }
        
        void Api::shutdown()
        {
            auto ctx = config::lock_settings();
            for (auto &c: cache)
                c->write(*ctx);
            
            shutdown_sync_worker();

            ObserverManager::clear<ApiObserver>();
        }
        
        void Api::start_playback(const std::string &album_id, const std::string &track_id)
        {
            Params params = {
                //{ "device_id", "" }
            };

            json o{
                { "context_uri", std::format("spotify:album:{}", album_id) },
                { "offset", {
                    { "uri", std::format("spotify:track:{}", track_id)} 
                }}
            };

            auto r = endpoint->Put(append_query_params(
                "/v1/me/player/play", params), o.dump(), "application/json");
            
            //ObserverManager::notify(&ApiObserver::on_track_changed, album_id, track_id);
        }
        
        void Api::skip_to_next()
        {
            // TODO: unfinished
            auto r = endpoint->Post("/v1/me/player/next");
        }

        void Api::skip_to_previous()
        {
            // TODO: unfinished
            auto r = endpoint->Post("/v1/me/player/previous");
        }

        void Api::toggle_shuffle(bool is_on)
        {
            auto r = endpoint->Put(append_query_params("/v1/me/player/shuffle",
                Params{{ "state", is_on ? "true" : "false" }}));
        }

        void Api::set_playback_volume(int volume_percent)
        {
            auto r = endpoint->Put(append_query_params("/v1/me/player/volume",
                Params{{ "volume_percent", std::to_string(volume_percent) }}));
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

            auto res = endpoint->Put("/v1/me/player", body.dump(), "application/json");
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
                auto r = endpoint->Get(request_url);

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
                auto r = endpoint->Get(request_url);

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
                auto r = endpoint->Get(request_url);

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
        
        const DevicesList& Api::get_available_devices() const
        {
            return dynamic_cast<DevicesCache&>(*cache[Devices]).get_data();
        }
        
        const PlaybackState& Api::get_playback_state() const
        {
            return dynamic_cast<PlaybackCache&>(*cache[Playback]).get_data();
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

                auto r = endpoint->Get("/v1/me/following", params, Headers());
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
                std::string exit_msg = "";
                auto marker = utils::clock::now();
                std::unique_lock<std::mutex> lock(sync_worker_mutex);

                try
                {
                    while (is_worker_listening)
                    {
                        // TODO: errors in this function raises on_playback_sync_finished, which is not valid
                        // for this situation, come up with some different errors handling
                        for (auto &c: cache)
                            c->resync();

                        // for the player to show track time ticking well, each frame starts as precise
                        // as possible to the 'marker' frame with 1s increment; in case for some reason 
                        // frame took more time to request and process data, we skip several of them
                        auto now = utils::clock::now();
                        while (marker < now)
                            marker += SYNC_INTERVAL;

                        std::this_thread::sleep_until(marker);
                    }
                }
                catch (const std::exception &ex)
                {
                    // TODO: what if there is an error, but n oplayback is opened
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
            std::unique_lock<std::mutex> lk(sync_worker_mutex);
            logger->info("An API sync worker has been stopped");
        }
    }
}