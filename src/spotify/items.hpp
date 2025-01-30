#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "stdafx.h"

namespace spotifar
{
    namespace spotify
    {
        static const string INVALID_ID = "";

        string make_item_uri(const string &item_type_name, const string &id);

        struct SimplifiedArtist
        {
            string id = INVALID_ID;
            wstring name;
            
            friend void from_json(const json &j, SimplifiedArtist &a);
            friend void to_json(json &j, const SimplifiedArtist &a);
        };

        struct Artist: public SimplifiedArtist
        {
            size_t popularity;

            friend void from_json(const json &j, Artist &a);
            friend void to_json(json &j, const Artist &a);
        };

        struct SimplifiedAlbum
        {
            inline static const string ALBUM = "album";
            inline static const string SINGLE = "single";
            inline static const string COMP = "compilation";
            inline static const string APPEARS_ON = "appears_on";

            string id = INVALID_ID;
            wstring name;
            size_t total_tracks;
            string album_type;
            string release_date;

            static string make_uri(const string &id) { return make_item_uri("album", id); }
            
            inline string get_uri() const { return make_uri(id); }
            inline bool is_single() const { return album_type == SINGLE; }
            string get_release_year() const;
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
            string id = INVALID_ID;
            wstring name;
            int duration_ms = 0;
            int duration = 0;
            size_t track_number;  // TODO: track number could be duplicated for different discs

            static string make_uri(const string &id) { return make_item_uri("track", id); }

            inline std::string get_uri() const { return make_uri(id); }
            friend bool operator==(const SimplifiedTrack &lhs, const SimplifiedTrack &rhs);
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

        struct SimplifiedPlaylist
        {
            string id = INVALID_ID;
            wstring name;
            wstring description;
            size_t tracks_total;

            static string make_uri(const string &id) { return make_item_uri("playlist", id); }

            inline std::string get_uri() const { return make_uri(id); }
            friend void from_json(const json &j, SimplifiedPlaylist &p);
        };

        struct Auth
        {
            string access_token;
            string scope;
            int expires_in;
            string refresh_token;
            
            friend void from_json(const json &j, Auth &a);
            friend void to_json(json &j, const Auth &a);
        };

        struct Actions
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

            friend bool operator==(const Actions &lhs, const Actions &rhs);
            friend void from_json(const json &j, Actions &p);
            friend void to_json(json &j, const Actions &p);
        };

        struct Context
        {
            inline static const string ALBUM = "album";
            inline static const string PLAYLIST = "playlist";
            inline static const string ARTIST = "artist";
            inline static const string SHOW = "show";
            inline static const string COLLECTION = "collection";

            string type;
            string uri;
            string href;

            bool is_empty() const { return type == ""; }
            bool is_artist() const { return type == ARTIST; }
            bool is_album() const { return type == ALBUM; }
            bool is_playlist() const { return type == PLAYLIST; }
            bool is_collection() const { return type == COLLECTION; }
            string get_item_id() const;

            friend bool operator==(const Context &lhs, const Context &rhs);

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(Context, type, uri, href);
        };

        struct Device
        {
            string id = INVALID_ID;
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
            inline  static const string REPEAT_OFF = "off";
            inline  static const string REPEAT_TRACK = "track";
            inline  static const string REPEAT_CONTEXT = "context";

            Device device;
            string repeat_state = REPEAT_OFF;
            bool shuffle_state = false;
            int progress_ms = 0;
            int progress = 0;
            bool is_playing = false;
            Actions actions;
            Track item;
            Context context;

            inline bool is_empty() const { return item.id == INVALID_ID; }
            friend void from_json(const json &j, PlaybackState &p);
            friend void to_json(json &j, const PlaybackState &p);
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
        typedef std::vector<Artist> ArtistsT;
        typedef std::map<string, SimplifiedPlaylist> PlaylistsCollection;
        typedef std::vector<Device> DevicesList;
        typedef std::vector<HistoryItem> HistoryList;
    }
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641