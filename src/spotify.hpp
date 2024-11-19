#ifndef SPOTIFY_B18B4445_D63E_4078_99D2_5A2B36197787
#define SPOTIFY_B18B4445_D63E_4078_99D2_5A2B36197787
#pragma once

// https://developer.spotify.com/documentation/web-api

#include "common/items.hpp"

#include <map>
#include <string>

#include "httplib.h"

namespace spotifar
{
	using std::map;
	using std::string;

	class SpotifyRelayApi
	{
	public:
		struct PlaybackDevice
		{
			string name;
			string id;
			bool is_active;
			short volume_percent;
		};

		typedef std::vector<PlaybackDevice> PlaybackDevices;
		
		struct Playlist
		{
			string name;
			int tracks_count;
			string tracks_request_url;
		};

		typedef std::vector<Playlist> Playlists;

	public:
		SpotifyRelayApi();

		bool login();

		std::vector<Artist> get_artists();

	protected:
		std::string get_api_uri() const;

	public:
		httplib::Client api;

		// cached data
		ArtistsCollection artists;
	};
}

#endif // SPOTIFY_B18B4445_D63E_4078_99D2_5A2B36197787

