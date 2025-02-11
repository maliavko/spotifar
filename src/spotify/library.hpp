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

    const std::vector<const artist*>& get_followed_artists() const { return followed_artists; }
    const artist* get_artist(const string &artist_id);
    const album* get_album(const string &album_id);
    const playlist* get_playlist(const string &playlist_id);

    // cached data interface
    virtual void resync(bool force = false);

private:
    bool is_initialized = false;
    api_abstract *api;
    
    std::vector<config::persistent_data_abstract*> storages;

    // TODO: convert into peersistent data maybe?
    std::unordered_map<string, artist> artists;
    std::unordered_map<string, album> albums;
    std::unordered_map<string, playlist> playlists;

    std::vector<const artist*> followed_artists;
};

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7