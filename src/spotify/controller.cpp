#include "controller.hpp"
#include "config.hpp"


namespace spotifar
{
    namespace api
    {
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

        const string Controller::SPOTIFY_AUTH_URL = "https://accounts.spotify.com";
        const string Controller::SPOTIFY_API_URL = "https://api.spotify.com";

        Controller::Controller(const string& client_id, const string& client_secret, int port,
                const string& refresh_token):
            api(SPOTIFY_API_URL),
            port(port),
            client_id(client_id),
            client_secret(client_secret),
            refresh_token(refresh_token),
            access_token_expires_at(0)
        {
            // TODO: add timer to refresh token
            // https://stackoverflow.com/questions/32233019/wake-up-a-stdthread-from-usleep
            // https://en.cppreference.com/w/cpp/thread/condition_variable
        }

        Controller::~Controller()
        {
        }

        bool Controller::authenticate()
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
        
        ArtistsCollection Controller::get_artist()
        {
            using json = nlohmann::json;

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
                break;
            }
            while (!after.is_null());

            return artists;
        }

        string Controller::request_auth_code()
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

            ShellExecuteA(NULL, "open", redirect_url.c_str(), 0, 0, SW_SHOW);

            return a.get();
        }

        bool Controller::update_access_token_with_auth_code(const string& auth_code)
        {
            return update_access_token(
                auth_code,
                httplib::Params{
                    { "grant_type", "authorization_code" },
                    { "code", auth_code },
                    { "redirect_uri", get_auth_callback_url() }
                }
            );
        }

        bool Controller::update_access_token_with_refresh_token(const string& refresh_token)
        {
            return update_access_token(
                refresh_token,
                httplib::Params{
                    { "grant_type", "refresh_token" },
                    { "refresh_token", refresh_token }
                }
            );
        }
        
        bool Controller::update_access_token(const string& token, const httplib::Params& params)
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
                refresh_token = data["refresh_token"];

            api.set_bearer_token_auth(access_token);

            return true;
        }
        
        std::string Controller::get_auth_callback_url() const
        {
            return std::format("http://localhost:{}/auth/callback", port);
        }
    }
}