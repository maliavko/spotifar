#include "auth.hpp"

namespace spotifar
{
    namespace spotify
    {
        const string SPOTIFY_AUTH_URL = "https://accounts.spotify.com";

        static string scope =
            "streaming "
            "playlist-read-private "
            "playlist-read-collaborative "
            "playlist-modify-private "
            "playlist-modify-public "
            "user-read-email "
            "user-read-private "
            "user-read-playback-state "
            "user-read-recently-played "
            "user-read-currently-playing "
            "user-modify-playback-state "
            "user-follow-read "
            "user-follow-modify "
            "user-library-read "
            "user-library-modify ";

        AuthCache::AuthCache(httplib::Client *endpoint,
                             const string &client_id, const string &client_secret, int port):
            CachedValue(endpoint, L"AccessToken"),
            client_id(client_id),
            client_secret(client_secret),
            port(port)
        {
            logger = spdlog::get(utils::LOGGER_API);
        };

        std::chrono::milliseconds AuthCache::get_sync_interval() const
        {
            // 60 seconds gap to overlap the old and the new tokens seemlessly
            return std::chrono::seconds(get_data().expires_in - 60);
        }
        
        void AuthCache::on_data_synced(const Auth &data, const Auth &prev_data)
        {
            logger->info("A valid access token is found, expires in {}",
                std::format("{:%T}", get_expires_at() - utils::clock::now()));
            endpoint->set_bearer_token_auth(data.access_token);
        }

        bool AuthCache::request_data(Auth &data)
        {
            auto refresh_token = get_data().refresh_token;
            // TODO: check errors
            if (!refresh_token.empty())
                data = auth_with_refresh_token(refresh_token);
            else
                data = auth_with_code(request_auth_code());
            return true;
        }
        
        Auth AuthCache::auth_with_code(const string &auth_code)
        {
            logger->info("Trying to obtain a spotify access token with auth code");
            return auth(
                httplib::Params{
                    { "grant_type", "authorization_code" },
                    { "code", auth_code },
                    { "redirect_uri", get_auth_callback_url() }
                }
            );
        }

        Auth AuthCache::auth_with_refresh_token(const string &refresh_token)
        {
            logger->info("Trying to obtain a spotify access token with stored refresh token");
            return auth(
                httplib::Params{
                    { "grant_type", "refresh_token" },
                    { "refresh_token", refresh_token }
                }
            );
        }
        
        Auth AuthCache::auth(const httplib::Params &params)
        {
            using json = nlohmann::json;

            httplib::Headers headers{
                { "Authorization", "Basic " + httplib::detail::base64_encode(client_id + ":" + client_secret) }
            };

            httplib::Client auth_endpoint(SPOTIFY_AUTH_URL);
            auto r = auth_endpoint.Post(
                "/api/token", headers, httplib::detail::params_to_query_str(params),
                "application/x-www-form-urlencoded");

            // TODO: error handling
           return json::parse(r->body).get<Auth>();
        }
        
        string AuthCache::request_auth_code()
        {
            // launching a http-server to receive an API auth reponse
            auto a = std::async(std::launch::async, [this]{
            	httplib::Server svr;
            	string result;

            	svr.Get("/auth/callback", [&](const httplib::Request& req, httplib::Response& res)
            	{
                    // TODO: add error handling, check "state"
            		result = req.get_param_value("code");
                    // TODO: consider of not closing the page, but pputting some info there
                    // e.g. "The login is finished successfully, now you can close the page"
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
        
        std::string AuthCache::get_auth_callback_url() const
        {
            return std::format("http://localhost:{}/auth/callback", port);
        }
    }
}