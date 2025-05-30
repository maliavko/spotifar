#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;

using json::from_json;
using json::to_json;
using item_id_t = string;
using item_ids_t = std::vector<item_id_t>;

static const item_id_t invalid_id = "";

/// @brief Returns spotify item's uri, like "spotify:track:6rqhFgbbKwnb9MLmUQDhG6"
/// @param item_type_name e.g. "track", "album", "playlist"
/// @param id spotify item id
string make_item_uri(const string &item_type_name, const string &id);

struct data_item_t
{
    item_id_t id = invalid_id;
    
    bool is_valid() const { return id != invalid_id; }

    friend bool operator==(const data_item_t &lhs, const data_item_t &rhs);
};

struct image_t
{
    string url;
    size_t width, height;

    friend void from_json(const json::Value &j, image_t &i);
    friend void to_json(json::Value &j, const image_t &i, json::Allocator &allocator);
};

struct simplified_artist_t: public data_item_t
{
    wstring name;

    auto get_uri() const -> string { return make_uri(id); }
    
    static auto make_uri(const item_id_t &id) -> string { return make_item_uri("artist", id); }
    
    friend void from_json(const json::Value &j, simplified_artist_t &a);
    friend void to_json(json::Value &j, const simplified_artist_t &a, json::Allocator &allocator);
};

struct artist_t: public simplified_artist_t
{
    size_t followers_total;
    size_t popularity;
    std::vector<string> genres;
    std::vector<image_t> images;

    friend void from_json(const json::Value &j, artist_t &a);
    friend void to_json(json::Value &j, const artist_t &a, json::Allocator &allocator);
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
    string release_date; // YYYY-MM-DD
    string href;
    std::vector<image_t> images;
    std::vector<simplified_artist_t> artists;

    static string make_uri(const item_id_t &id) { return make_item_uri("album", id); }
    
    auto get_uri() const -> string { return make_uri(id); }
    auto is_single() const -> bool { return album_type == single; }
    auto is_compilation() const -> bool { return album_type == compilation; }
    auto get_release_year() const -> string;
    auto get_release_date() const -> utils::clock_t::time_point;
    auto get_type_abbrev() const -> wstring;
    
    friend void from_json(const json::Value &j, simplified_album_t &a);
    friend void to_json(json::Value &j, const simplified_album_t &a, json::Allocator &allocator);
};

struct album_t: public simplified_album_t
{
    friend void from_json(const json::Value &j, album_t &a);
    friend void to_json(json::Value &j, const album_t &a, json::Allocator &allocator);
};

struct saved_album_t: public album_t
{
    string added_at;
    
    friend void from_json(const json::Value &j, saved_album_t &a);
    friend void to_json(json::Value &j, saved_album_t &a, json::Allocator &allocator);
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

    static string make_uri(const item_id_t &id) { return make_item_uri("track", id); }
    static const string& get_fields_filter();

    inline string get_uri() const { return make_uri(id); }
    
    friend void from_json(const json::Value &j, simplified_track_t &t);
    friend void to_json(json::Value &j, const simplified_track_t &t, json::Allocator &allocator);
};

struct track_t: public simplified_track_t
{
    album_t album;

    static const string& get_fields_filter();

    /// @brief Takes a first available artists or returns a sturb string 
    wstring get_artist_name() const;

    /// @brief Returns a string, containing all the track's artists separated by comma 
    wstring get_artists_full_name() const;

    friend void from_json(const json::Value &j, track_t &t);
    friend void to_json(json::Value &j, const track_t &t, json::Allocator &allocator);
};

struct saved_track_t: public track_t
{
    string added_at;
    
    static const string& get_fields_filter();
    
    friend void from_json(const json::Value &j, saved_track_t &t);
    friend void to_json(json::Value &result, const saved_track_t &t, json::Allocator &allocator);
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

    static string make_uri(const item_id_t &id) { return make_item_uri("playlist", id); }
    static const string& get_fields_filter();

    inline string get_uri() const { return make_uri(id); }
    
    friend void from_json(const json::Value &j, simplified_playlist_t &p);
    friend void to_json(json::Value &j, const simplified_playlist_t &a, json::Allocator &allocator);
};

struct playlist_t: public simplified_playlist_t
{
    static const string& get_fields_filter();
    
    friend void from_json(const json::Value &j, playlist_t &p);
    friend void to_json(json::Value &j, const playlist_t &a, json::Allocator &allocator);
};

// true - allowed, false - disallowed
struct actions_t
{
    // default flags represent the state of the player's UI controls,
    // when nothing is being played: we can resume playback and we can tranfer a device
    bool interrupting_playback = false;
    bool pausing = false;
    bool resuming = true;
    bool seeking = false;
    bool skipping_next = false;
    bool skipping_prev = false;
    bool toggling_repeat_context = false;
    bool toggling_repeat_track = false;
    bool toggling_shuffle = false;
    bool trasferring_playback = true;

    friend bool operator==(const actions_t &lhs, const actions_t &rhs);
    
    friend void from_json(const json::Value &j, actions_t &a);
    friend void to_json(json::Value &j, const actions_t &a, json::Allocator &allocator);
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
    
    friend void from_json(const json::Value &j, context_t &c);
    friend void to_json(json::Value &j, const context_t &c, json::Allocator &allocator);
};

struct device_t: public data_item_t
{
    string id = "";
    bool is_active = false;
    wstring name;
    string type;
    int volume_percent = 100;
    bool supports_volume = false;
    bool is_private_session = false;
    bool is_restricted = false;

    string to_str() const;
    
    friend void from_json(const json::Value &j, device_t &d);
    friend void to_json(json::Value &j, const device_t &d, json::Allocator &allocator);
};

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
    
    friend void from_json(const json::Value &j, playback_state_t &p);
    friend void to_json(json::Value &j, const playback_state_t &p, json::Allocator &allocator);
};

struct history_item_t
{
    track_t track;
    context_t context;
    string played_at;
    
    friend void from_json(const json::Value &j, history_item_t &i);
    friend void to_json(json::Value &j, const history_item_t &i, json::Allocator &allocator);
};

struct playing_queue_t
{
    track_t currently_playing;
    std::vector<track_t> queue;
    
    friend void from_json(const json::Value &j, playing_queue_t &i);
    friend void to_json(json::Value &j, const playing_queue_t &i, json::Allocator &allocator);
};

struct auth_t
{
    string access_token;
    string scope;
    int expires_in;
    string refresh_token;
    
    bool is_valid() const { return !access_token.empty(); }
    
    friend void from_json(const json::Value &j, auth_t &a);
    friend void to_json(json::Value &j, const auth_t &a, json::Allocator &allocator);
};

using devices_t = std::vector<device_t>;
using history_items_t = std::vector<history_item_t>;
using recent_releases_t = std::vector<simplified_album_t>;

} // namespace spotify
} // namespace spotifar

template<>
struct std::hash<spotifar::spotify::simplified_album_t>
{
    std::size_t operator()(const spotifar::spotify::simplified_album_t &item) const
    {
        std::size_t res = 0;
        spotifar::utils::combine(res, std::hash<string>{}(item.id));
        return res;
    }
};

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641