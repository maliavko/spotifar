#ifndef SPOTIFY_HTTP_RELAY_HPP_F0151191_7FCF_4109_B14E_390787F85559
#define SPOTIFY_HTTP_RELAY_HPP_F0151191_7FCF_4109_B14E_390787F85559

#pragma once

#include <string>

namespace spotifar
{
	using std::string;
	using httplib::Server;
	using httplib::Client;
	using httplib::Request;
	using httplib::Response;

	class SpotifyHttpRelay
	{
	public:
		const string SPOTIFY_AUTH_URI = "https://accounts.spotify.com";
		const string SPOTIFY_API_URI = "https://api.spotify.com";

		SpotifyHttpRelay(int port, const string& client_id, const string& client_secret);

		bool listen();

	protected:
		string dump_headers(const httplib::Headers&);

		string log(const httplib::Request&, const httplib::Response&);

		bool exchange_token(string auth_code, bool is_refresh_token);

		std::string get_auth_callback_href() const;

		void refresh_user_info();

		bool refresh_auth_token(httplib::Response& res);

		string preprocess_api_command(const std::string& cmd);

	private:
		Server svr;
		Client auth, api;

		int port;
		string client_id;
		string client_secret;

		string api_token;
		std::time_t api_token_expires_at;
		string refresh_token;

		string user_name;
		string user_id;

		std::map<string, std::function<string()>> macroses;
	};
}

#endif //SPOTIFY_HTTP_RELAY_HPP_F0151191_7FCF_4109_B14E_390787F85559
