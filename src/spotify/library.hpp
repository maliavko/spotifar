#ifndef LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#define LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "abstract.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

class LibraryCache: public cached_data_abstract
{
public:
    LibraryCache(api_abstract *api);
    virtual ~LibraryCache();

    // persistent data interface
    virtual void read(settings_ctx &ctx);
    virtual void write(settings_ctx &ctx);
    virtual void clear(settings_ctx &ctx);

    artists_t get_followed_artists();

    artist get_artist(const string &artist_id);
    albums_t get_artist_albums(const string &artist_id);
    tracks_t get_artist_top_tracks(const string &artist_id);

    album get_album(const string &album_id);
    simplified_tracks_t get_album_tracks(const string &album_id);

    playlist get_playlist(const string &playlist_id);
    simplified_playlists_t get_playlists();
    playlist_tracks_t get_playlist_tracks(const string &playlist_id);

    // cached data interface
    virtual void resync(bool force = false);

private:
    bool is_initialized = false;
    api_abstract *api;
    
    std::vector<config::persistent_data_abstract*> storages;

    // TODO: convert into peersistent data maybe?
    // std::unordered_map<string, artist> artists;
    // std::unordered_map<string, album> albums;
    // std::unordered_map<string, playlist> playlists;

    // std::vector<const artist*> followed_artists;
};

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7