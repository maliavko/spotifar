#include "stdafx.h"
#include "spotify_http_relay.hpp"

namespace spotifar
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

	using json = nlohmann::json;

	struct Artist
	{
		std::string name;
		size_t popularity;
	};

	void to_json(json& j, const Artist& p) {
		j = json{ {"name", p.name}, {"popularity", p.popularity} };
	}

	void from_json(const json& j, Artist& p) {
		j.at("name").get_to(p.name);
		j.at("popularity").get_to(p.popularity);
	}

	SpotifyHttpRelay::SpotifyHttpRelay(int port_, const string& client_id_,
		const string& client_secret_
	):
		auth(SPOTIFY_AUTH_URI),
		port(port_),
		client_id(client_id_),
		client_secret(client_secret_),
		api(SPOTIFY_API_URI),
		api_token_expires_at(0)
	{
		auth.set_follow_location(true);

		macroses.insert({
			{ "user-id", [this]() { return user_id; } },
		});

		svr.set_logger([this](const Request& req, const Response& res)
			{
				printf("%s", log(req, res).c_str());
			});

		svr.Get("/auth/callback", [this](const Request& req, Response& res)
			{
				exchange_token(req.get_param_value("code"), false);
				res.set_content("<script>window.close();</script>", "text/html");

				refresh_user_info();
			});
		
		svr.Get("/login", [this](const Request& req, Response& res)
			{
				refresh_auth_token(res);
			});
		
		svr.Get("/artists", [this](const Request& req, Response& res)
			{
				using json = nlohmann::json;

				json after = "";
				std::vector<Artist> artists;

				do
				{
					httplib::Params params = {
						{ "type", "artist" },
						{ "limit", "50" },
						{ "after", after.get<std::string>() },
					};

					auto r = api.Get("/v1/me/following", params, httplib::Headers());
					json data = json::parse(r->body)["artists"];

					for (json& a : data["items"])
					{
						artists.push_back(a.get<Artist>());
					}

					after = data["cursors"]["after"];
				}
				while (!after.is_null());

				std::sort(artists.begin(), artists.end(), [](const auto& a, const auto& b) { return a.name < b.name; });

				res.set_content(json(artists).dump(), "application/json; charset=utf-8");
			});

		svr.Get("/invoke", [this](const Request& req, Response& res)
			{
				auto cmd = req.params.find("cmd");
				auto method = req.params.find("method");

				if (cmd == req.params.end() or method == req.params.end())
					return;

				httplib::Params params;
				for (const auto& p : req.params)
					if (p.first != "cmd" && p.first != "method")
						params.insert(p);

				std::string command = preprocess_api_command(cmd->second);

				httplib::Result r;
				if (method->second == "put")
					r = api.Put(command, params);
				else
					r = api.Get(command, params, httplib::Headers());

				res.status = r->status;
				res.set_content(r->body, "application/json; charset=utf-8");
			});
	}

	bool SpotifyHttpRelay::listen()
	{
		return svr.listen("localhost", port);
	}

	bool SpotifyHttpRelay::refresh_auth_token(Response& res)
	{
		// requesting access token only in case it is needed
		if (std::time(nullptr) < api_token_expires_at)
			return false;

		// prolonging api auth token with refresh token if possible
		if (!refresh_token.empty())
			return exchange_token(refresh_token, true);

		httplib::Params params{
			{ "response_type", "code" },
			{ "client_id", client_id },
			{ "scope", scope },
			{ "redirect_uri", get_auth_callback_href() },
			{ "state", utils::generate_random_string(16) },
		};

		string redirect_url = httplib::append_query_params(
			"https://accounts.spotify.com/authorize/?", params);

		ShellExecuteA(NULL, "open", redirect_url.c_str(), 0, 0, SW_SHOW);

		//res.set_redirect(redirect_url);

		return true;
	}

	void SpotifyHttpRelay::refresh_user_info()
	{
		using json = nlohmann::json;

		auto r = api.Get("/v1/me");

		json info = json::parse(r->body);
		user_name = info["display_name"].get<string>();
		user_id = info["id"].get<string>();
	}

	bool SpotifyHttpRelay::exchange_token(string token, bool is_refresh_token)
	{
		using std::cout;
		using std::endl;
		using json = nlohmann::json;

		httplib::Params params;

		if (is_refresh_token)
		{
			params.emplace("grant_type", "refresh_token");
			params.emplace("refresh_token", token);
		}
		else
		{
			params.emplace("grant_type", "authorization_code");
			params.emplace("code", token);
			params.emplace("redirect_uri", get_auth_callback_href());
		}

		httplib::Headers headers{
			{ "Authorization", "Basic " + httplib::detail::base64_encode(client_id + ":" + client_secret) }
		};

		auto r = auth.Post("/api/token", headers,
			httplib::detail::params_to_query_str(params),
			"application/x-www-form-urlencoded");

		cout << "Error: " << httplib::to_string(r.error()) << endl;
		cout << "Status: " << r->status << endl;
		cout << "Body: " << r->body << endl;

		json data = json::parse(r->body);

		api_token = data["access_token"];
		api_token_expires_at = std::time(nullptr) + data["expires_in"].get<int>();

		if (data.contains("refresh_token"))
			refresh_token = data["refresh_token"];

		api.set_bearer_token_auth(api_token);

		return true;
	}

	std::string SpotifyHttpRelay::get_auth_callback_href() const
	{
		return std::format("http://localhost:{}/auth/callback", port);
	}

	string SpotifyHttpRelay::preprocess_api_command(const std::string& cmd)
	{
		string result = cmd;

		// going through all the macroses
		for (const auto& macro : macroses)
		{
			// replacing all the occurances of the macro in the string
			size_t pos = result.find(macro.first);
			while (pos != string::npos)
			{
				string replace_by = macro.second();
				result.replace(pos, macro.first.size(), replace_by);
				pos = result.find(macro.first, pos + replace_by.size());
			}
		}

		return std::format("/v1{}", result);
	}

	string SpotifyHttpRelay::dump_headers(const httplib::Headers& headers)
	{
		std::string s;
		char buf[BUFSIZ];

		for (auto it = headers.begin(); it != headers.end(); ++it)
		{
			const auto& x = *it;
			snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
			s += buf;
		}

		return s;
	}

	string SpotifyHttpRelay::log(const Request& req, const Response& res)
	{
		std::string s;
		char buf[BUFSIZ];

		s += "================================\n";

		snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
			req.version.c_str(), req.path.c_str());
		s += buf;

		std::string query;
		for (auto it = req.params.begin(); it != req.params.end(); ++it)
		{
			const auto& x = *it;
			snprintf(buf, sizeof(buf), "%c%s=%s",
				(it == req.params.begin()) ? '?' : '&', x.first.c_str(),
				x.second.c_str());
			query += buf;
		}
		snprintf(buf, sizeof(buf), "%s\n", query.c_str());
		s += buf;

		s += dump_headers(req.headers);

		s += "--------------------------------\n";

		snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
		s += buf;
		s += dump_headers(res.headers);
		s += "\n";

		if (!res.body.empty()) { s += res.body; }

		s += "\n";

		return s;
	}
}
