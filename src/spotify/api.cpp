#include "api.hpp"
#include "config.hpp"


namespace spotifar
{
    namespace spotify
    {
        using json = nlohmann::json;

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

        Api::Api(const string& client_id, const string& client_secret, int port,
                const string& refresh_token):
            api(SPOTIFY_API_URL),
            port(port),
            client_id(client_id),
            client_secret(client_secret),
            refresh_token(refresh_token),
            access_token_expires_at(0),
            is_listening(false),
            devices(std::make_unique<DevicesList>())
        {
            // TODO: add timer to refresh token
            // https://stackoverflow.com/questions/32233019/wake-up-a-stdthread-from-usleep
            // https://en.cppreference.com/w/cpp/thread/condition_variable
        }

        Api::~Api()
        {
            devices = nullptr;
        }

        bool Api::authenticate()
        {
            // requesting access token only in case it is needed
            if (std::time(nullptr) < access_token_expires_at)
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
        
        void Api::shutdown()
        {
            while (observers.size())
                stop_listening(*observers.begin());
        }
        
        void Api::start_playback(const std::string& album_id, const std::string& track_id)
        {
            httplib::Params params = {
                { "device_id", "ce8d71004f9597141d4b5940bd1bb2dc52a35dae" }  // TODO: add a normal detection of device id
            };

            json o{
                { "context_uri", std::format("spotify:album:{}", album_id) },
                { "offset", {
                    { "uri", std::format("spotify:track:{}", track_id)} 
                }}
            };

            auto r = api.Put(httplib::append_query_params("/v1/me/player/play", params), o.dump(), "application/json");
        }
        
        void Api::skip_to_next()
        {
            // TODO: unfinished
            auto r = api.Post("/v1/me/player/next");
        }

        void Api::skip_to_previous()
        {
            // TODO: unfinished
            auto r = api.Post("/v1/me/player/previous");
        }

        void Api::toggle_shuffle(bool is_on)
        {
            auto r = api.Put(httplib::append_query_params("/v1/me/player/shuffle",
                httplib::Params{{ "state", is_on ? "true" : "false" }}));
        }

        void Api::set_playback_volume(int volume_percent)
        {
            auto s = httplib::append_query_params("/v1/me/player/volume",
                httplib::Params{{ "volume_percent", std::to_string(volume_percent) }});
            auto r = api.Put(httplib::append_query_params("/v1/me/player/volume",
                httplib::Params{{ "volume_percent", std::to_string(volume_percent) }}));
        }
        
        AlbumsCollection Api::get_albums(const std::string& artist_id)
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
                auto r = api.Get(request_url);

                json data = json::parse(r->body);
                for (json& aj : data["items"])
                {
                    auto a = aj.get<Album>();
                    albums[a.id] = a;
                }

                json next = data["next"];
                if (next.is_null())
                    break;
                
                next.get_to(request_url);
            } while (1);

            return albums;
        }
        
        std::map<string, SimplifiedTrack> Api::get_tracks(const std::string& album_id)
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
                auto r = api.Get(request_url);

                json data = json::parse(r->body);
                for (json& tj : data["items"])
                {
                    auto t = tj.get<Track>();
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

                auto r = api.Get("/v1/me/following", params, httplib::Headers());
                json data = json::parse(r->body)["artists"];

                for (json& aj : data["items"])
                {
                    auto a = aj.get<Artist>();
                    artists[a.id] = a;
                }
                after = data["cursors"]["after"];
                break; // TODO: remove, used for debugging
            }
            while (!after.is_null());

            return artists;
        }
        
        PlaybackState Api::get_playback_state()
        {
            PlaybackState state;  // empty playback by default

            auto r = api.Get("/v1/me/player");
            if (r->status == httplib::OK_200)    
            {
                state = json::parse(r->body).get<PlaybackState>();
            }

            return state;
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

            spdlog::info("Requesting spotify auth code, redirecting to the external browser");
            ShellExecuteA(NULL, "open", redirect_url.c_str(), 0, 0, SW_SHOW);

            return a.get();
        }

        void Api::request_available_devices(DevicesList& devices_in)
        {
            if (auto r = api.Get("/v1/me/player/devices"))
                json::parse(r->body).at("devices").get_to(devices_in);
        }

        bool Api::update_access_token_with_auth_code(const string& auth_code)
        {
            spdlog::info("Trying to obtain a spotify auth token with auth code");
            return update_access_token(
                auth_code,
                httplib::Params{
                    { "grant_type", "authorization_code" },
                    { "code", auth_code },
                    { "redirect_uri", get_auth_callback_url() }
                }
            );
        }

        bool Api::update_access_token_with_refresh_token(const string& refresh_token)
        {
            spdlog::info("Trying to obtain a spotify auth token with stored refresh token");
            return update_access_token(
                refresh_token,
                httplib::Params{
                    { "grant_type", "refresh_token" },
                    { "refresh_token", refresh_token }
                }
            );
        }
        
        bool Api::update_access_token(const string& token, const httplib::Params& params)
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
            access_token_expires_at = std::time(nullptr) + data["expires_in"].get<int>();

            if (data.contains("refresh_token"))
            {
                refresh_token = data["refresh_token"];
                spdlog::info("A refresh token is found and stored");
            }

            api.set_bearer_token_auth(access_token);
            spdlog::info("An auth token is received successfully");

            return true;
        }
    
        void Api::start_listening(ApiProtocol* observer)
        {
            ObserverManager::subscribe<ApiProtocol>(observer);
            observers.push_back(observer);

            if (is_listening)
                return;

            is_listening = true;
            std::packaged_task<void()> task([this, observer]
            {
                auto marker = std::chrono::high_resolution_clock::now();   
                is_in_sync_with_api = true;

                try
                {
                    while (is_listening)
                    {
                        resync_caches();

                        // for the player to show track time ticking well, each frame starts as precise
                        // as possible to the 'marker' frame with 1s increment; in case for some reason 
                        // frame took more time to request and process data, we skip several of them
                        auto now = std::chrono::high_resolution_clock::now();
                        while (marker < now)
                            marker += SYNC_INTERVAL;

                        std::this_thread::sleep_until(marker);
                    }
                    is_in_sync_with_api = false;
                    cv.notify_all();
                }
                catch (const std::exception& ex)
                {
                    ObserverManager::notify(&ApiProtocol::on_playback_sync_failed, ex.what());
                    return;
                }
            });

            std::thread(std::move(task)).detach();
        }

        void Api::stop_listening(ApiProtocol* observer)
        {
            auto it = std::find(observers.begin(), observers.end(), observer);
            if (it == observers.end())
                return;
            
            observers.erase(it);
            ObserverManager::unsubscribe<ApiProtocol>(observer);

            if (is_listening && !observers.size())
            {
                is_listening = false;

                // manual object lifetime management, due to detached thread,
                // lets hold object untill the last sync is completed
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [this]{ return !is_in_sync_with_api; });
            }
        }

        void Api::resync_caches()
        {
            // updating available devices list
            DevicesList new_devices;
            request_available_devices(new_devices);
            bool has_devices_changed = !std::equal(new_devices.begin(), new_devices.end(),
                                                   devices->begin(), devices->end(),
                                                   [](const auto& a, const auto& b) { return a.id == b.id && a.is_active == b.is_active; });
            
            if (has_devices_changed)
            {
                devices->assign(new_devices.begin(), new_devices.end());
                ObserverManager::notify(&ApiProtocol::on_devices_changed, *devices);
            }
    
            // TODO: make a diff and invoke the updates in particular,
            // note: after executing some command like play or set volume the diff can 
            // go to UI right away, to avoid desync artefacts
            //auto state = get_playback_state();
            //ObserverManager::notify(&ApiProtocol::on_playback_updated, state);
        }
        
        std::string Api::get_auth_callback_url() const
        {
            return std::format("http://localhost:{}/auth/callback", port);
        }
    }
}