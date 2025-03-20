#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "stdafx.h"

// TODO: to make a review of the methods, now some items have methods for formatting data,
// which possibly is needed only for views
namespace spotifar { namespace spotify {

static const string invalid_id = "";

/// @brief Returns spotify item's uri, like "spotify:track:6rqhFgbbKwnb9MLmUQDhG6"
/// @param item_type_name e.g. "track", "album", "playlist"
/// @param id spotify item id
string make_item_uri(const string &item_type_name, const string &id);

struct data_item_t
{
    string id = invalid_id;
};

struct image_t
{
    string url;
    size_t width, height;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(image_t, url, width, height);
};

struct simplified_artist_t: public data_item_t
{
    wstring name;

    inline auto get_uri() const -> string { return make_uri(id); }
    inline auto is_valid() const -> bool { return id != invalid_id; }
    
    static auto make_uri(const string &id) -> string { return make_item_uri("artist", id); }

    friend void from_json(const json &j, simplified_artist_t &a);
    friend void to_json(json &j, const simplified_artist_t &a);
};

struct artist_t: public simplified_artist_t
{
    size_t followers_total;
    size_t popularity;
    std::vector<string> genres;
    std::vector<image_t> images;

    friend void from_json(const json &j, artist_t &a);
    friend void to_json(json &j, const artist_t &a);
};

struct simplified_album_t: public data_item_t
{
    inline static const string
        album = "album",
        single = "single",
        compilation = "compilation",
        appears_on = "appears_on";

    wstring name;
    size_t total_tracks;
    string album_type;
    string release_date;
    string release_date_precision; // "year", "month", "day"
    string href;
    std::vector<image_t> images;
    std::vector<simplified_artist_t> artists;

    static string make_uri(const string &id) { return make_item_uri("album", id); }
    
    inline bool is_valid() const { return id != invalid_id; }
    inline string get_uri() const { return make_uri(id); }
    inline bool is_single() const { return album_type == single; }
    string get_release_year() const;
    wstring get_type_abbrev() const;
    wstring get_user_name() const;
    friend void from_json(const json &j, simplified_album_t &a);
    friend void to_json(json &j, const simplified_album_t &p);
};

struct album_t: public simplified_album_t
{
    friend void from_json(const json &j, album_t &t);
    friend void to_json(json &j, const album_t &p);
};

struct saved_album_t: public album_t
{
    string added_at;
    
    friend void from_json(const json &j, saved_album_t &a);
    friend void to_json(json &j, const saved_album_t &a);
};

struct simplified_track_t: public data_item_t
{
    wstring name;
    int duration_ms = 0;
    int duration = 0;
    size_t disc_number;
    size_t track_number;
    bool is_explicit;
    std::vector<simplified_artist_t> artists;

    static string make_uri(const string &id) { return make_item_uri("track", id); }
    static const string& get_fields_filter();

    inline bool is_valid() const { return id != invalid_id; }
    inline string get_uri() const { return make_uri(id); }
    friend bool operator==(const simplified_track_t &lhs, const simplified_track_t &rhs);
    friend void from_json(const json &j, simplified_track_t &t);
    friend void to_json(json &j, const simplified_track_t &t);
};

struct track_t: public simplified_track_t
{
    album_t album;
    std::vector<simplified_artist_t> artists;

    static const string& get_fields_filter();

    wstring get_artist_name() const;
    wstring get_artists_full_name() const;
    wstring get_long_name() const;

    friend void from_json(const json &j, track_t &t);
    friend void to_json(json &j, const track_t &p);
};

struct saved_track_t: public track_t
{
    string added_at;
    
    static const string& get_fields_filter();
    
    friend void from_json(const json &j, saved_track_t &t);
    friend void to_json(json &j, const saved_track_t &t);
};

struct simplified_playlist_t: public data_item_t
{
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
    friend void from_json(const json &j, simplified_playlist_t &p);
};

struct playlist_t: public simplified_playlist_t
{
    //std::vector<saved_track_t> tracks;
    static const string& get_fields_filter();
};

struct actions_t
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

    friend bool operator==(const actions_t &lhs, const actions_t &rhs);
    friend void from_json(const json &j, actions_t &p);
    friend void to_json(json &j, const actions_t &p);
};

struct context_t
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

    friend bool operator==(const context_t &lhs, const context_t &rhs);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(context_t, type, uri, href);
};

struct device_t
{
    string id = "";
    bool is_active = false;
    wstring name;
    string type;
    int volume_percent = 100;
    bool supports_volume = false;

    string to_str() const;
    friend bool operator==(const device_t &lhs, const device_t &rhs);
    friend void from_json(const json &j, device_t &d);
    friend void to_json(json &j, const device_t &d);
};

// https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
struct playback_state_t
{
    inline static const string
        repeat_off = "off",
        repeat_track = "track",
        repeat_context = "context";

    actions_t actions;
    device_t device;
    string repeat_state = repeat_off;
    bool shuffle_state = false;
    int progress_ms = 0;
    int progress = 0;
    bool is_playing = false;
    track_t item;
    context_t context;

    inline bool is_empty() const { return item.id == ""; }
    friend void from_json(const json &j, playback_state_t &p);
    friend void to_json(json &j, const playback_state_t &p);
};

struct history_item_t
{
    track_t track;
    context_t context;
    string played_at;
    
    friend void from_json(const json &j, history_item_t &p);
    friend void to_json(json &j, const history_item_t &p);
};

typedef std::vector<device_t> devices_t;
typedef std::vector<artist_t> artists_t;
typedef std::vector<simplified_album_t> simplified_albums_t;
typedef std::vector<album_t> albums_t;
typedef std::vector<saved_album_t> saved_albums_t;
typedef std::vector<simplified_track_t> simplified_tracks_t;
typedef std::vector<track_t> tracks_t;
typedef std::vector<saved_track_t> saved_tracks_t;
typedef std::vector<simplified_playlist_t> simplified_playlists_t;
typedef std::vector<playlist_t> playlists_t;
typedef std::vector<history_item_t> history_items_t;
typedef std::vector<std::pair<string, track_t>> recent_tracks_t;

struct playing_queue_t
{
    track_t currently_playing;
    tracks_t queue;
    
    friend void from_json(const json &j, playing_queue_t &p);
    friend void to_json(json &j, const playing_queue_t &p);
};

} // namespace spotify
} // namespace spotifar

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641