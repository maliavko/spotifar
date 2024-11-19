#include "stdafx.h"
#include "spotify.hpp"
#include "config.hpp"

namespace spotifar
{
	using nlohmann::json;
	using std::string;

	SpotifyRelayApi::SpotifyRelayApi():
		api(get_api_uri())
	{
	}

	bool SpotifyRelayApi::login()
	{
		auto r = api.Get("/login");
		return r.value().status == httplib::StatusCode::OK_200;
	}

	std::vector<Artist> SpotifyRelayApi::get_artists()
	{
		auto r = api.Get("/artists");
		return json::parse(r->body).get<std::vector<Artist>>();
	}

	/*bool SpotifyRelayApi::is_playing()
	{
		auto r = api.Get("me/player/currently-playing");
		return json::parse(r->body).value("is_playing", false);
	}

	bool SpotifyRelayApi::play()
	{
		api.Put("me/player/pause");
		return true;
	}

	bool SpotifyRelayApi::pause()
	{
		api.Put("me/player/play");
		return true;
	}

	bool SpotifyRelayApi::toggle_play()
	{
		if (is_playing())
			pause();
		else
			play();

		return true;
	}

	SpotifyRelayApi::PlaybackDevices SpotifyRelayApi::get_devices_list()
	{
		PlaybackDevices result;

		auto r = api.Get("me/player/devices");

		json devices = json::parse(r->body)["devices"];
		for (json::iterator it = devices.begin(); it != devices.end(); ++it)
		{
			auto& item = it.value();
			result.push_back(PlaybackDevice{
				item.at("name").get<string>(),
				item.at("id").get<string>(),
				item.at("is_active").get<bool>(),
				item.at("volume_percent").get<short>(),
			});
		}

		return result;
	}

	SpotifyRelayApi::Playlists SpotifyRelayApi::get_playlists()
	{
		Playlists result;

		//auto r = api.Get("users/" + user_info.id + "/playlists");

		//json playlists = json::parse(r->body)["items"];
		//for (json::iterator it = playlists.begin(); it != playlists.end(); ++it)
		//{
		//	auto& item = it.value();
		//	result.push_back(Playlist{
		//		item["name"].get<string>(),
		//		item["tracks"]["total"].get<int>(),
		//		item["tracks"]["href"].get<string>(),
		//	});
		//}

		return result;
	}

	bool SpotifyRelayApi::select_device(const std::string& device_id)
	{
		std::vector<std::string> result;

		json j{
			{ "device_ids", json::array({ device_id }) }
		};

		auto rs = api.Put("player", j.dump(), "application/json");

		return true;
	}*/

	std::string SpotifyRelayApi::get_api_uri() const
	{
		return std::format("http://localhost:{}", config::Opt.LocalhostServicePort);
	}
}
