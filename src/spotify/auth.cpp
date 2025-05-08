#include "auth.hpp"
#include "lng.hpp"

namespace spotifar { namespace spotify {

using namespace utils;

static string
    spotify_auth_url = "https://accounts.spotify.com",
    scope =
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

/// @brief Randomg string generator function, stolen from Spotify API documentation examples. Used
/// for additional security step 
static string generate_random_string(const int length)
{
    static const string possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    string text = "";
    for (int i = 0; i < length; i++)
    {
        float rand = (float)std::rand() / RAND_MAX * possible.length();
        text += possible[(int)std::floor(rand)];
    }
    return text;
};

auth_cache::auth_cache(api_interface *api, const string &client_id, const string &client_secret, int port):
    json_cache(L"AccessToken"),
    client_id(client_id),
    client_secret(client_secret),
    port(port),
    api_proxy(api)
{
};

void auth_cache::shutdown(config::settings_context &ctx)
{
    json_cache::shutdown(ctx);

    if (auth_server.is_running())
        auth_server.stop();
}

clock_t::duration auth_cache::get_sync_interval() const
{
    // 60 seconds gap to overlap the old and the new tokens seemlessly
    return std::chrono::seconds(get().expires_in - 60);
}

void auth_cache::on_data_synced(const auth_t &data, const auth_t &prev_data)
{
    log::api->info("A valid access token is found, expires in {}",
        std::format("{:%T}", get_expires_at() - clock_t::now()));
    
    // utils::far3::synchro_tasks::dispatch_event(&auth_observer::on_auth_status_changed, data, is_logged_in);

    // calling through synchro event postpones the event for a quite some time, hope this call in
    // the sync-thread will not bring me troubles
    ObserverManager::notify(&auth_observer::on_auth_status_changed, data, is_logged_in);

    is_logged_in = true;
}

bool auth_cache::request_data(auth_t &data)
{
    auto refresh_token = get().refresh_token;
    if (!refresh_token.empty())
    {
        data = auth_with_refresh_token(refresh_token);

        // an api does not return refresh token in case it is still valid,
        // so putting the old one manually to the data cache
        if (data.refresh_token.empty())
            data.refresh_token = refresh_token;
    }
    else
        data = auth_with_code(request_auth_code());
    return true;
}

auth_t auth_cache::auth_with_code(const string &auth_code)
{
    log::api->info("Trying to obtain a spotify access token with auth code");
    return authenticate(
        httplib::Params{
            { "grant_type", "authorization_code" },
            { "code", auth_code },
            { "redirect_uri", get_auth_callback_url() }
        }
    );
}

auth_t auth_cache::auth_with_refresh_token(const string &refresh_token)
{
    log::api->info("Trying to obtain a spotify access token with stored refresh token");
    return authenticate(
        httplib::Params{
            { "grant_type", "refresh_token" },
            { "refresh_token", refresh_token }
        }
    );
}

auth_t auth_cache::authenticate(const httplib::Params &params)
{
    auth_t result{};

    httplib::Headers headers{
        { "Authorization", "Basic " + httplib::detail::base64_encode(client_id + ":" + client_secret) }
    };
    
    httplib::Client auth_endpoint(spotify_auth_url);
    auth_endpoint.set_logger(http_logger);

    auto res = auth_endpoint.Post(
        "/api/token", headers, httplib::detail::params_to_query_str(params),
        "application/x-www-form-urlencoded");

    // any error while authorization means critical error, they are being caught in the sync-thread,
    // which will lead to showing an error dialog and closing plugin
    if (!http::is_success(res))
        throw std::runtime_error(std::format("Authorization error: {}", http::get_status_message(res)));

    json::parse_to(res->body, result);

    return result;
}

string auth_cache::request_auth_code()
{
    assert(!auth_server.is_running());

    auto state = generate_random_string(16);
    
    // making a request through user's system browser, as he has to
    // login and provide an auth coockie to complete request
    httplib::Params params{
        { "response_type", "code" },
        { "client_id", client_id },
        { "scope", scope }, 
        { "redirect_uri", get_auth_callback_url() },
        { "state",  state },
    };

    string redirect_url = httplib::append_query_params(
        spotify_auth_url + "/authorize/", params);

    log::api->info("Requesting spotify auth code, redirecting to the external browser");
    ShellExecuteA(NULL, "open", redirect_url.c_str(), 0, 0, SW_SHOW);

    // launching a http-server to receive an API auth reponse
    string result, result_msg = "could not retrive an access token for unknown reason";

    auth_server.Get("/auth/callback", [&](const httplib::Request &req, httplib::Response &res)
    {
        auto response_state = req.get_param_value("state");

        if (response_state == state)
        {
            result = req.get_param_value("code");
            if (result.empty())
            {
                res.status = httplib::StatusCode::InternalServerError_500;
                result_msg = "received access code is empty";
            }
            else
            {
                res.status = httplib::StatusCode::OK_200;
                result_msg = "access token is retrieved successfully";
            }
        }
        else
        {
            res.status = httplib::StatusCode::InternalServerError_500;
            result_msg = std::format("state field does not match, requested '{}', "
                "received '{}'", state, response_state);
        }

        res.set_content(std::format("<h1>{}, {}</h1><p>{}</p>",
            httplib::status_message(res.status), res.status, result_msg), "text/html");
        auth_server.stop();
    });

    // a waiting timeout for the user to enter his credentials. Works well, not sure
    // this functionality is needed at all
    // auto a = std::async(std::launch::async, [this]
    // {
    //     // preliminary 1200 iterations by 25 milliseconds ~30 seconds, no need in
    //     // precision here
    //     for (size_t i = 0; i < 1200; i++)
    //     {
    //         std::this_thread::sleep_for(std::chrono::milliseconds(25));

    //         // in case the server is stopped - we finilize a handler
    //         if (!auth_server.is_running())
    //             return;
    //     }
    //     auth_server.stop();
    // });

    auth_server.listen("127.0.0.1", port);

    if (result.empty())
        throw std::runtime_error(std::format("Authentication error: '{}'", result_msg));

    return result;
}

string auth_cache::get_auth_callback_url() const
{
    return std::format("http://127.0.0.1:{}/auth/callback", port);
}

} // namespace spotify
} // namespace spotifar