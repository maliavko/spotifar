#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "nlohmann\json.hpp"
#include <string>
#include <map>

namespace spotifar
{
	namespace spotify
	{
		using std::map;
		using std::vector;
		using std::string;
		using std::wstring;
		using json = nlohmann::json;

		struct SimplifiedArtist
		{
			string id;
			string name;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimplifiedArtist, id, name);
		};

		struct Artist: public SimplifiedArtist
		{
			size_t popularity;
			
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Artist, id, name, popularity);
		};

		struct Album
		{
			string id;
			string name;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Album, id, name);
		};

		struct SimplifiedTrack
		{
			string id;
			string name;
			size_t track_number;  // TODO: track number could be duplicated for different discs
			
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimplifiedTrack, id, name, track_number);
		};

		struct Track: public SimplifiedTrack
		{
			Album album;
			std::vector<SimplifiedArtist> artists;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Track, id, name, track_number, album, artists);
		};

		struct Permissions
		{
			bool interrupting_playback;
			bool pausing;
			bool resuming;
			bool seeking;
			bool skipping_next;
			bool skipping_prev;
			bool toggling_repeat_context;
			bool toggling_repeat_track;
			bool toggling_shuffle;
			bool trasferring_playback;
			
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Permissions, interrupting_playback, pausing, resuming, seeking,
				skipping_next, skipping_prev, toggling_repeat_context, toggling_repeat_track,
				toggling_shuffle, trasferring_playback);
		};

		struct Context
		{
			inline static const string ALBUM = "album";
			inline static const string PLAYLIST = "playlist";
			inline static const string ARTIST = "artist";
			inline static const string SHOW = "show";

			string type;
			
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Context, type);
		};

		struct Device
		{
			string id;
			bool is_active;
			string name;
			string type;
			int volume_percent;
			bool supports_volume;
			wstring user_name;
			std::shared_ptr<Context> context;

			friend bool operator==(const Device& lhs, const Device& rhs);
			friend void from_json(const json& j, Device& d);
		};

		// https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
		struct PlaybackState
		{
			Device device;
			string repeat_state;  // off, track, context
			bool shuffle_state;
			size_t progress_ms;
			bool is_playing;
			Permissions actions;
			std::shared_ptr<Track> track = nullptr;
			std::shared_ptr<Context> context = nullptr;

			inline bool is_empty() const { return track == nullptr; }
			friend void from_json(const json& j, PlaybackState& p);
		};

		typedef map<string, Album> AlbumsCollection;
		typedef map<string, Artist> ArtistsCollection;
		typedef vector<Device> DevicesList;
	}
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641