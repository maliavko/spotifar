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
		using std::string;
		using std::wstring;
		using json = nlohmann::json;

		struct Auth
		{
			string access_token;
			string scope;
			int expires_in;
			string refresh_token;
			
			friend void from_json(const json &j, Auth &a);
			friend void to_json(json &j, const Auth &a);
		};

		struct SimplifiedArtist
		{
			string id;
			wstring name;
			
			friend void from_json(const json &j, SimplifiedArtist &a);
			friend void to_json(json &j, const SimplifiedArtist &p);
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
			
			inline bool is_single() const { return album_type == SINGLE; }
			friend void from_json(const json &j, SimplifiedAlbum &a);
			friend void to_json(json &j, const SimplifiedAlbum &p);
		};

		struct Album: public SimplifiedAlbum
		{
			friend void from_json(const json &j, Album &t);
			friend void to_json(json &j, const Album &p);
		};

		struct SimplifiedTrack
		{
			string id;
			wstring name;
			size_t duration;  // in seconds
			size_t track_number;  // TODO: track number could be duplicated for different discs

			friend void from_json(const json &j, SimplifiedTrack &t);
			friend void to_json(json &j, const SimplifiedTrack &t);
		};

		struct Track: public SimplifiedTrack
		{
			Album album;
			std::vector<SimplifiedArtist> artists;

			friend void from_json(const json &j, Track &t);
			friend void to_json(json &j, const Track &p);
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
			friend void to_json(json &j, const Permissions &p);
		};

		struct Context
		{
			inline static const string ALBUM = "album";
			inline static const string PLAYLIST = "playlist";
			inline static const string ARTIST = "artist";
			inline static const string SHOW = "show";

			string type;
			string uri;
			
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Context, type, uri);
		};

		struct Device
		{
			string id;
			bool is_active = false;
			wstring name;
			string type;
			int volume_percent = 100;
			bool supports_volume = false;
 
			std::string to_str() const;
			friend bool operator==(const Device &lhs, const Device &rhs);
			friend void from_json(const json &j, Device &d);
			friend void to_json(json &j, const Device &d);
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
			friend void to_json(json &j, const PlaybackState &p);
		};

		struct SimplifiedPlaylist
		{
			string id;
			wstring name;
			wstring description;
			size_t tracks_total;

			friend void from_json(const json &j, SimplifiedPlaylist &p);
		};
		
		struct HistoryItem
		{
			Track track;
			Context context;
			string played_at;
			
			friend void from_json(const json &j, HistoryItem &p);
			friend void to_json(json &j, const HistoryItem &p);
		};
		
		typedef std::map<string, SimplifiedAlbum> AlbumsCollection;
		typedef std::map<string, Artist> ArtistsCollection;
		typedef std::map<string, SimplifiedPlaylist> PlaylistsCollection;
		typedef std::vector<Device> DevicesList;
		typedef std::vector<HistoryItem> HistoryList;
	}
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641