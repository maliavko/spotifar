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

struct simplified_artist
{
    string id = invalid_id;
    wstring name;
    
    friend void from_json(const json &j, simplified_artist &a);
    friend void to_json(json &j, const simplified_artist &a);
};

struct artist: public simplified_artist
{
    size_t popularity;

    friend void from_json(const json &j, artist &a);
    friend void to_json(json &j, const artist &a);
};

struct SimplifiedAlbum
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
    
    inline string get_uri() const { return make_uri(id); }
    inline bool is_single() const { return album_type == single; }
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
    string id = invalid_id;
    wstring name;
    int duration_ms = 0;
    int duration = 0;
    size_t track_number;  // TODO: track number could be duplicated for different discs

    static string make_uri(const string &id) { return make_item_uri("track", id); }
    static const string& get_fields_filter();
    inline string get_uri() const { return make_uri(id); }
    friend bool operator==(const SimplifiedTrack &lhs, const SimplifiedTrack &rhs);
    friend void from_json(const json &j, SimplifiedTrack &t);
    friend void to_json(json &j, const SimplifiedTrack &t);
};

struct Track: public SimplifiedTrack
{
    Album album;
    std::vector<simplified_artist> artists;

    static const string& get_fields_filter();
    friend void from_json(const json &j, Track &t);
    friend void to_json(json &j, const Track &p);
};

struct SimplifiedPlaylist
{
    string id = invalid_id;
    wstring name;
    wstring description;
    size_t tracks_total;

    static string make_uri(const string &id) { return make_item_uri("playlist", id); }

    inline string get_uri() const { return make_uri(id); }
    friend void from_json(const json &j, SimplifiedPlaylist &p);
};

struct PlaylistTrack
{
    string added_at;
    Track track;
    
    static const string& get_fields_filter();
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PlaylistTrack, added_at, track);
};

typedef std::map<string, SimplifiedAlbum> AlbumsCollection;
typedef std::map<string, artist> ArtistsCollection;
typedef std::vector<artist> ArtistsT;
typedef std::vector<Track> TracksT;
typedef std::vector<PlaylistTrack> PlaylistTracksT;
typedef std::vector<SimplifiedTrack> SimplifiedTracksT;
typedef std::map<string, SimplifiedPlaylist> PlaylistsCollection;

} // namespace spotify
} // namespace spotifar

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641