#include "auth.hpp"

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

auth_cache::auth_cache(api_abstract *api, const string &client_id, const string &client_secret, int port):
    json_cache(L"AccessToken"),
    client_id(client_id),
    client_secret(client_secret),
    port(port),
    api_proxy(api)
{
};

clock_t::duration auth_cache::get_sync_interval() const
{
    // 60 seconds gap to overlap the old and the new tokens seemlessly
    return std::chrono::seconds(get().expires_in - 60);
}

void auth_cache::on_data_synced(const auth &data, const auth &prev_data)
{
    log::api->info("A valid access token is found, expires in {}",
        std::format("{:%T}", get_expires_at() - clock_t::now()));

    is_logged_in = true;
    
    // log::api->debug("Access token: {}", data.access_token);
    utils::far3::synchro_tasks::dispatch_event(&auth_observer::on_auth_status_changed, data);
}

bool auth_cache::request_data(auth &data)
{
    auto refresh_token = get().refresh_token;
    // TODO: check errors
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

auth auth_cache::auth_with_code(const string &auth_code)
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

auth auth_cache::auth_with_refresh_token(const string &refresh_token)
{
    log::api->info("Trying to obtain a spotify access token with stored refresh token");
    return authenticate(
        httplib::Params{
            { "grant_type", "refresh_token" },
            { "refresh_token", refresh_token }
        }
    );
}

auth auth_cache::authenticate(const httplib::Params &params)
{
    httplib::Headers headers{
        { "Authorization", "Basic " + httplib::detail::base64_encode(client_id + ":" + client_secret) }
    };
    
    httplib::Client auth_endpoint(spotify_auth_url);
    auto res = auth_endpoint.Post(
        "/api/token", headers, httplib::detail::params_to_query_str(params),
        "application/x-www-form-urlencoded");

    // TODO: error handling
    return json::parse(res->body).get<auth>();
}

string auth_cache::request_auth_code()
{
    // launching a http-server to receive an API auth reponse
    auto a = std::async(std::launch::async, [this] {
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

        svr.listen("127.0.0.1", port);
        return result;
    });
    
    // making a request through user's system browser, as he has to
    // login and provide an auth coockie to complete request
    httplib::Params params{
        { "response_type", "code" },
        { "client_id", client_id },
        { "scope", scope }, 
        { "redirect_uri", get_auth_callback_url() },
        { "state", generate_random_string(16) },
    };

    string redirect_url = httplib::append_query_params(
        spotify_auth_url + "/authorize/", params);

    log::api->info("Requesting spotify auth code, redirecting to the external browser");
    ShellExecuteA(NULL, "open", redirect_url.c_str(), 0, 0, SW_SHOW);

    return a.get();
}

string auth_cache::get_auth_callback_url() const
{
    return std::format("http://127.0.0.1:{}/auth/callback", port);
}

void from_json(const json &j, auth &a)
{
    j.at("access_token").get_to(a.access_token);
    j.at("scope").get_to(a.scope);
    j.at("expires_in").get_to(a.expires_in);

    a.refresh_token = j.value("refresh_token", "");
}

void to_json(json &j, const auth &a)
{
    j = json{
        { "access_token", a.access_token },
        { "scope", a.scope },
        { "expires_in", a.expires_in },
        { "refresh_token", a.refresh_token },
    };
}

} // namespace spotify
} // namespace spotifar