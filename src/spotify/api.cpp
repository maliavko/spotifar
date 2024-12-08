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
            is_listening(false)
        {
            // TODO: add timer to refresh token
            // https://stackoverflow.com/questions/32233019/wake-up-a-stdthread-from-usleep
            // https://en.cppreference.com/w/cpp/thread/condition_variable
        }

        Api::~Api()
        {
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
            PlaybackState state;

            auto r = api.Get("/v1/me/player");
            if (r->status == httplib::OK_200)    
            {
                state = json::parse(r->body).get<PlaybackState>();
            }

            return state;
        }

        DevicesList Api::get_available_devices()
        {
            DevicesList devices;

            if (auto r = api.Get("/v1/me/player/devices"))
                json::parse(r->body).at("devices").get_to(devices);

            return devices;
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
            // TODO: sync threads?
            std::packaged_task<void()> task([this, observer]
            {
                static DevicesList devices;

                try
                {
                    // TODO: thread can process data while application is closing, some data could be
                    // already invalid. Repro steps: close far in the middle of syncs
                    while (is_listening)
                    {
                        // updating available devices list
                        auto new_devices = get_available_devices();
                        if (devices != new_devices)
                        {
                            ObserverManager::notify(&ApiProtocol::on_devices_changed, new_devices);
                            devices = new_devices;
                        }
                
                        auto state = get_playback_state();
                        ObserverManager::notify(&ApiProtocol::on_playback_updated, state);

                        std::this_thread::sleep_for(SYNC_INTERVAL);
                    }
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
                is_listening = false;
        }
        
        std::string Api::get_auth_callback_url() const
        {
            return std::format("http://localhost:{}/auth/callback", port);
        }
    }
}