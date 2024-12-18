#include "api.hpp"
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

        static string scope =
            "streaming "
            "user-read-email "
            "user-read-private "
            "user-read-playback-state "
            "user-modify-playback-state "
            "user-read-currently-playing "
            "user-follow-read "
            "user-follow-modify "
            "user-library-read "
            "user-library-modify "
            "playlist-read-private "
            "playlist-read-collaborative ";

        const string Api::SPOTIFY_AUTH_URL = "https://accounts.spotify.com";
        const string Api::SPOTIFY_API_URL = "https://api.spotify.com";

        Api::Api(const string &client_id, const string &client_secret, int port,
                const string &refresh_token):
            endpoint(std::make_unique<Client>(SPOTIFY_API_URL)),
            port(port),
            client_id(client_id),
            client_secret(client_secret),
            refresh_token(refresh_token),
            access_token_expires_at({}),
            is_worker_listening(false),
            devices(std::make_unique<DevicesList>()),
            logger(spdlog::get(utils::LOGGER_API))
        {
            endpoint->set_logger([this](const Request &req, const Response &res)
            {
                if (res.status != OK_200 && res.status != NoContent_204)
                    logger->error(dump_http_error(req, res));
            });
            
            endpoint->set_default_headers(httplib::Headers{
                {"Content-Type", "application/json; charset=utf-8"},
            });
        }

        Api::~Api()
        {
            devices = nullptr;
            logger = nullptr;
            endpoint = nullptr;
        }

        bool Api::init()
        {
            launch_sync_worker();
            return true;
        }
        
        void Api::shutdown()
        {
            shutdown_sync_worker();

            ObserverManager::clear<ApiObserver>();
            active_observers.clear();
        }
        
        void Api::start_playback(const std::string &album_id, const std::string &track_id)
        {
            httplib::Params params = {
                //{ "device_id", "" }
            };

            json o{
                { "context_uri", std::format("spotify:album:{}", album_id) },
                { "offset", {
                    { "uri", std::format("spotify:track:{}", track_id)} 
                }}
            };

            auto r = endpoint->Put(httplib::append_query_params(
                "/v1/me/player/play", params), o.dump(), "application/json");
            
            ObserverManager::notify(&ApiObserver::on_track_changed, album_id, track_id);
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
            auto r = endpoint->Put(httplib::append_query_params("/v1/me/player/shuffle",
                httplib::Params{{ "state", is_on ? "true" : "false" }}));
        }

        void Api::set_playback_volume(int volume_percent)
        {
            auto r = endpoint->Put(httplib::append_query_params("/v1/me/player/volume",
                httplib::Params{{ "volume_percent", std::to_string(volume_percent) }}));
        }
        
        bool Api::transfer_playback(const std::string &device_id, bool start_playing)
        {   
            auto device_it = std::find_if(devices->begin(), devices->end(),
                [&device_id](auto &d) { return d.id == device_id; });

            if (device_it == devices->end())
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

            httplib::Params params = {
                { "limit", "50" },
                { "offset", "0" },
                { "include_groups", "album" }
            };

            std::string request_url = httplib::append_query_params(
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
        
        std::map<string, SimplifiedTrack> Api::get_tracks(const std::string &album_id)
        {
            std::map<string, SimplifiedTrack> tracks;

            httplib::Params params = {
                { "limit", "50" },
                { "offset", "0" }
            };

            std::string request_url = httplib::append_query_params(
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
        
        ArtistsCollection Api::get_artist()
        {
            json after = "";
            ArtistsCollection artists;

            do
            {
                httplib::Params params = {
                    { "type", "artist" },
                    { "limit", "50" },
                    { "after", after.get<std::string>() },
                };

                auto r = endpoint->Get("/v1/me/following", params, httplib::Headers());
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
        
        PlaybackState Api::get_playback_state()
        {
            PlaybackState state;  // empty playback by default

            auto r = endpoint->Get("/v1/me/player");
            if (r->status == httplib::OK_200)    
            {
                state = json::parse(r->body).get<PlaybackState>();
            }

            return state;
        }

        bool Api::authenticate()
        {
            // requesting access token only in case it is needed
            if (clock::now() < access_token_expires_at)
                return true;

            // prolonging api auth token with refresh token if possible
            if (!refresh_token.empty())
            {
                // TODO: check errors
                update_access_token_with_refresh_token(refresh_token);
                return true;
            }

            // TODO: errors
            return update_access_token_with_auth_code(request_auth_code());
        }
        
        string Api::request_auth_code()
        {
            // launching a http-server to receive an API auth reponse
            auto a = std::async(std::launch::async, [this]{
            	httplib::Server svr;
            	string result;

            	svr.Get("/auth/callback", [&](const httplib::Request& req, httplib::Response& res)
            	{
                    // TODO: add error handling, check "state"
            		result = req.get_param_value("code");
                    // if success, we are closing an empty page automatically
				    res.set_content("<script>window.close();</script>", "text/html");
            		svr.stop();
            	});

            	svr.listen("localhost", port);
                return result;
            });
            
            // making a request through user's system browser, as he has to
            // login and provide an auth coockie to complete request
            httplib::Params params{
                { "response_type", "code" },
                { "client_id", client_id },
                { "scope", scope },
                { "redirect_uri", get_auth_callback_url() },
                { "state", utils::generate_random_string(16) },
            };

            string redirect_url = httplib::append_query_params(
                SPOTIFY_AUTH_URL + "/authorize/", params);

            logger->info("Requesting spotify auth code, redirecting to the external browser");
            ShellExecuteA(NULL, "open", redirect_url.c_str(), 0, 0, SW_SHOW);

            return a.get();
        }

        void Api::request_available_devices(DevicesList &devices_in)
        {
            if (auto r = endpoint->Get("/v1/me/player/devices"))
                json::parse(r->body).at("devices").get_to(devices_in);
        }

        bool Api::update_access_token_with_auth_code(const string &auth_code)
        {
            logger->info("Trying to obtain a spotify auth token with auth code");
            return update_access_token(
                auth_code,
                httplib::Params{
                    { "grant_type", "authorization_code" },
                    { "code", auth_code },
                    { "redirect_uri", get_auth_callback_url() }
                }
            );
        }

        bool Api::update_access_token_with_refresh_token(const string &refresh_token)
        {
            logger->info("Trying to obtain a spotify auth token with stored refresh token");
            return update_access_token(
                refresh_token,
                httplib::Params{
                    { "grant_type", "refresh_token" },
                    { "refresh_token", refresh_token }
                }
            );
        }
        
        bool Api::update_access_token(const string &token, const httplib::Params &params)
        {
            using json = nlohmann::json;

            httplib::Headers headers{
                { "Authorization", "Basic " + httplib::detail::base64_encode(client_id + ":" + client_secret) }
            };

            httplib::Client auth_endpoint(SPOTIFY_AUTH_URL);
            auto r = auth_endpoint.Post(
                "/api/token", headers, httplib::detail::params_to_query_str(params),
                "application/x-www-form-urlencoded"
                );

            // TODO: error handling
            json data = json::parse(r->body);

            access_token = data["access_token"];
            access_token_expires_at = clock::now() + std::chrono::seconds(data["expires_in"].get<int>()) -
                ACCESS_TOKEN_REFRESH_GAP;

            if (data.contains("refresh_token"))
            {
                refresh_token = data["refresh_token"];
                logger->info("A refresh token is found and stored");
            }

            endpoint->set_bearer_token_auth(access_token);
            logger->info("An auth token is received successfully");

            return true;
        }

        void Api::launch_sync_worker()
        {
            std::packaged_task<void()> task([this]
            {
                std::string exit_msg = "";
                auto marker = clock::now();
                std::unique_lock<std::mutex> lock(sync_worker_mutex);

                try
                {
                    while (is_worker_listening)
                    {
                        // TODO: handler errors
                        authenticate();

                        if (active_observers.size())
                            resync_caches();

                        // for the player to show track time ticking well, each frame starts as precise
                        // as possible to the 'marker' frame with 1s increment; in case for some reason 
                        // frame took more time to request and process data, we skip several of them
                        auto now = clock::now();
                        while (marker < now)
                            marker += SYNC_INTERVAL;

                        std::this_thread::sleep_until(marker);
                    }
                }
                catch (const std::exception &ex)
                {
                    exit_msg = ex.what();
                }
                
                ObserverManager::notify(&ApiObserver::on_playback_sync_finished, exit_msg);
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
    
        void Api::start_listening(ApiObserver *o, bool is_active)
        {
            ObserverManager::subscribe<ApiObserver>(o);

            if (is_active)
                active_observers.insert(o);
        }

        void Api::stop_listening(ApiObserver *o)
        {
            auto it = active_observers.find(o);
            if (it != active_observers.end())
                active_observers.erase(it);
            
            ObserverManager::unsubscribe<ApiObserver>(o);
        }

        void Api::resync_caches()
        {
            // updating available devices list
            DevicesList new_devices;
            request_available_devices(new_devices);
            bool has_devices_changed = !std::equal(new_devices.begin(), new_devices.end(),
                                                   devices->begin(), devices->end(),
                                                   [](const auto &a, const auto &b) { return a.id == b.id && a.is_active == b.is_active; });
            
            if (has_devices_changed)
            {
                devices->assign(new_devices.begin(), new_devices.end());
                ObserverManager::notify(&ApiObserver::on_devices_changed, *devices);
            }
    
            // TODO: make a diff and invoke the updates in particular,
            // note: after executing some command like play or set volume the diff can 
            // go to UI right away, to avoid desync artefacts
            auto state = get_playback_state();
            ObserverManager::notify(&ApiObserver::on_playback_updated, state);
        }
        
        std::string Api::get_auth_callback_url() const
        {
            return std::format("http://localhost:{}/auth/callback", port);
        }
    }
}