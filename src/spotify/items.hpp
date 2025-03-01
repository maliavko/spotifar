#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "stdafx.h"

namespace spotifar { namespace spotify {

static const string invalid_id = "";

/// @brief Returns spotify item's uri, like "spotify:track:6rqhFgbbKwnb9MLmUQDhG6"
/// @param item_type_name e.g. "track", "album", "playlist"
/// @param id spotify item id
string make_item_uri(const string &item_type_name, const string &id);

struct image
{
    string url;
    size_t width, height;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(image, url, width, height);
};

struct simplified_artist
{
    string id = invalid_id;
    wstring name;
    
    static string make_uri(const string &id) { return make_item_uri("artist", id); }

    inline string get_uri() const { return make_uri(id); }
    inline bool is_valid() const { return id != invalid_id; }
    friend void from_json(const json &j, simplified_artist &a);
    friend void to_json(json &j, const simplified_artist &a);
};

struct artist: public simplified_artist
{
    size_t followers_total;
    size_t popularity;
    std::vector<string> genres;
    std::vector<image> images;

    friend void from_json(const json &j, artist &a);
    friend void to_json(json &j, const artist &a);
};

struct simplified_album
{
    inline static const string
        album = "album",
        single = "single",
        compilation = "compilation",
        appears_on = "appears_on";

    string id = invalid_id;
    wstring name;
    size_t total_tracks;
    string album_type;
    string release_date;

    static string make_uri(const string &id) { return make_item_uri("album", id); }
    
    inline bool is_valid() const { return id != invalid_id; }
    inline string get_uri() const { return make_uri(id); }
    inline bool is_single() const { return album_type == single; }
    string get_release_year() const;
    wstring get_user_name() const;
    friend void from_json(const json &j, simplified_album &a);
    friend void to_json(json &j, const simplified_album &p);
};

struct album: public simplified_album
{
    friend void from_json(const json &j, album &t);
    friend void to_json(json &j, const album &p);
};

struct simplified_track
{
    string id = invalid_id;
    wstring name;
    int duration_ms = 0;
    int duration = 0;
    size_t track_number;  // TODO: track number could be duplicated for different discs

    static string make_uri(const string &id) { return make_item_uri("track", id); }
    static const string& get_fields_filter();

    inline bool is_valid() const { return id != invalid_id; }
    inline string get_uri() const { return make_uri(id); }
    friend bool operator==(const simplified_track &lhs, const simplified_track &rhs);
    friend void from_json(const json &j, simplified_track &t);
    friend void to_json(json &j, const simplified_track &t);
};

struct track: public simplified_track
{
    album album;
    std::vector<simplified_artist> artists;

    static const string& get_fields_filter();

    wstring get_artist_name() const;
    wstring get_artists_full_name() const;
    wstring get_long_name() const;

    friend void from_json(const json &j, track &t);
    friend void to_json(json &j, const track &p);
};

struct playlist_track
{
    string added_at;
    track track;
    
    static const string& get_fields_filter();
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(playlist_track, added_at, track);
};

struct simplified_playlist
{
    string id = invalid_id;
    string href;
    string snapshot_id;
    wstring name;
    wstring user_display_name;
    bool collaborative;
    bool is_public;
    wstring description;
    size_t tracks_total;

    static string make_uri(const string &id) { return make_item_uri("playlist", id); }
    static const string& get_fields_filter();

    inline bool is_valid() const { return id != invalid_id; }
    inline string get_uri() const { return make_uri(id); }
    friend void from_json(const json &j, simplified_playlist &p);
};

struct playlist: public simplified_playlist
{
    //std::vector<playlist_track> tracks;
    static const string& get_fields_filter();
};

struct actions
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

    friend bool operator==(const actions &lhs, const actions &rhs);
    friend void from_json(const json &j, actions &p);
    friend void to_json(json &j, const actions &p);
};

struct context
{
    inline static const string
        album = "album",
        playlist = "playlist",
        artist = "artist",
        show = "show",
        collection = "collection";

    string type;
    string uri;
    string href;

    bool is_empty() const { return type == ""; }
    bool is_artist() const { return type == artist; }
    bool is_album() const { return type == album; }
    bool is_playlist() const { return type == playlist; }
    bool is_collection() const { return type == collection; }
    string get_item_id() const;

    friend bool operator==(const context &lhs, const context &rhs);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(context, type, uri, href);
};

struct device
{
    string id = "";
    bool is_active = false;
    wstring name;
    string type;
    int volume_percent = 100;
    bool supports_volume = false;

    string to_str() const;
    friend bool operator==(const device &lhs, const device &rhs);
    friend void from_json(const json &j, device &d);
    friend void to_json(json &j, const device &d);
};

// https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
struct playback_state
{
    inline static const string
        repeat_off = "off",
        repeat_track = "track",
        repeat_context = "context";

    actions actions;
    device device;
    string repeat_state = repeat_off;
    bool shuffle_state = false;
    int progress_ms = 0;
    int progress = 0;
    bool is_playing = false;
    track item;
    context context;

    inline bool is_empty() const { return item.id == ""; }
    friend void from_json(const json &j, playback_state &p);
    friend void to_json(json &j, const playback_state &p);
};

struct playing_queue
{
    track currently_playing;
    std::vector<track> queue;
    
    friend void from_json(const json &j, playing_queue &p);
    friend void to_json(json &j, const playing_queue &p);
};

typedef std::vector<device> devices_t;
typedef std::vector<artist> artists_t;
typedef std::vector<simplified_album> albums_t;
typedef std::vector<simplified_track> simplified_tracks_t;
typedef std::vector<track> tracks_t;
typedef std::vector<playlist_track> playlist_tracks_t;
typedef std::vector<simplified_playlist> simplified_playlists_t;
typedef std::vector<playlist> playlists_t;

} // namespace spotify
} // namespace spotifar

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641