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

		struct ApiDataItem
		{
			virtual std::string to_str() const = 0;
		};

		struct SimplifiedArtist: public ApiDataItem
		{
			string id;
			wstring name;
			
			virtual std::string to_str() const;
			friend void from_json(const json &j, SimplifiedArtist &a);
		};

		struct Artist: public SimplifiedArtist
		{
			size_t popularity;

			friend void from_json(const json &j, Artist &a);
		};

		struct SimplifiedAlbum
		{
			inline static const string ALBUM = "album";
			inline static const string SINGLE = "single";
			inline static const string COMP = "compilation";
			inline static const string APPEARS_ON = "appears_on";

			string id;
			wstring name;
			size_t total_tracks;
			string album_type;
			string release_date;
			string release_year;
			//string album_group;  // suspiciously the attribute is not present in the parent object
			
			virtual std::string to_str() const;
			inline bool is_single() const { return album_type == SINGLE; }
			friend void from_json(const json &j, SimplifiedAlbum &a);
		};

		struct Album: public SimplifiedAlbum
		{
			friend void from_json(const json &j, Album &t);
		};

		struct SimplifiedTrack
		{
			string id;
			wstring name;
			size_t duration;  // in seconds
			size_t track_number;  // TODO: track number could be duplicated for different discs

			friend void from_json(const json &j, SimplifiedTrack &t);
		};

		struct Track: public SimplifiedTrack
		{
			Album album;
			std::vector<SimplifiedArtist> artists;

			friend void from_json(const json &j, Track &t);
		};

		struct Permissions
		{
			bool interrupting_playback = false;
			bool pausing = false;
			bool resuming = false;
			bool seeking = false;
			bool skipping_next = false;
			bool skipping_prev = false;
			bool toggling_repeat_context = false;
			bool toggling_repeat_track = false;
			bool toggling_shuffle = false;
			bool trasferring_playback = false;

			friend void from_json(const json &j, Permissions &p);
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
			bool is_active = false;
			wstring name;
			string type;
			int volume_percent = 100;
			bool supports_volume = false;

			virtual std::string to_str() const;
			friend bool operator==(const Device &lhs, const Device &rhs);
			friend void from_json(const json &j, Device &d);
		};

		// https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
		struct PlaybackState
		{
			inline static const string REPEAT_OFF = "off";
			inline static const string REPEAT_TRACK = "track";
			inline static const string REPEAT_CONTEXT = "context";

			Device device;
			string repeat_state = REPEAT_OFF;  // off, track, context
			bool shuffle_state = false;
			size_t progress = 0; // in seconds
			bool is_playing = false;
			Permissions permissions;
			std::shared_ptr<Track> track = nullptr;
			std::shared_ptr<Context> context = nullptr;

			inline bool is_empty() const { return track == nullptr; }
			friend void from_json(const json &j, PlaybackState &p);
		};

		typedef map<string, SimplifiedAlbum> AlbumsCollection;
		typedef map<string, Artist> ArtistsCollection;
		typedef vector<Device> DevicesList;
	}
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641