#ifndef REQUESTS_HPP_40324BB1_0A71_4030_8792_1F98850B6550
#define REQUESTS_HPP_40324BB1_0A71_4030_8792_1F98850B6550
#pragma once

#include "abstract.hpp"

namespace spotifar { namespace spotify {

static const size_t MAX_LIMIT = 50;

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-followed
struct followed_artists_requester: public api_collection_requester<artists_t>
{
    followed_artists_requester(size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/me/following", {
            { "type", "artist" },
            { "limit", std::to_string(limit) },
        }, "artists")
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-albums
struct saved_albums_requester: public api_collection_requester<saved_albums_t>
{
    saved_albums_requester(size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/me/albums", {
            { "limit", std::to_string(limit) }
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-new-releases
struct new_releases_requester: public api_collection_requester<simplified_albums_t>
{
    new_releases_requester(size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/browse/new-releases", {
            { "limit", std::to_string(limit) }
        }, "albums")
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
struct saved_tracks_requester: public api_collection_requester<saved_tracks_t>
{
    saved_tracks_requester(size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/me/tracks", {
            { "limit", std::to_string(limit) }
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artist
struct artist_requester: public api_requester<artist_t>
{
    artist_requester(const string &artist_id):
        api_requester(std::format("/v1/artists/{}", artist_id))
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-top-tracks
struct artist_top_tracks_requester: public api_requester<tracks_t>
{
    artist_top_tracks_requester(const string &artist_id):
        api_requester(std::format("/v1/artists/{}/top-tracks", artist_id))
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-albums
struct artist_albums_requester: public api_collection_requester<simplified_albums_t>
{
    artist_albums_requester(const string &artist_id, size_t limit = MAX_LIMIT):
        api_collection_requester(std::format("/v1/artists/{}/albums", artist_id), {
            { "include_groups", "album" },
            { "limit", std::to_string(limit) },
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-album
struct album_requester: public api_requester<album_t>
{
    album_requester(const string &album_id):
        api_requester(std::format("/v1/albums/{}", album_id))
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-albums-tracks
struct album_tracks_requester: public api_collection_requester<simplified_tracks_t>
{
    album_tracks_requester(const string &album_id, size_t limit = MAX_LIMIT):
        api_collection_requester(std::format("/v1/albums/{}/tracks", album_id), {
            { "limit", std::to_string(limit) },
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-list-users-playlists
struct user_playlists_requester: public api_collection_requester<simplified_playlists_t>
{
    user_playlists_requester(size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/me/playlists", {
            { "limit", std::to_string(limit) },
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-playlist
struct playlist_requester: public api_requester<playlist_t>
{
    playlist_requester(const string &playlist_id):
        api_requester(std::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist_t::get_fields_filter() },
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-playlists-tracks
struct playlist_tracks_requester: public api_collection_requester<saved_tracks_t>
{
    playlist_tracks_requester(const string &playlist_id, size_t limit = MAX_LIMIT):
        api_collection_requester(std::format("/v1/playlists/{}/tracks", playlist_id), {
            { "additional_types", "track" },
            { "fields", std::format("items({}),next,total", saved_track_t::get_fields_filter()) },
            { "limit", std::to_string(limit) },
        })
        {}
};

} // namespace spotify
} // namespace spotifar

#endif // REQUESTS_HPP_40324BB1_0A71_4030_8792_1F98850B6550