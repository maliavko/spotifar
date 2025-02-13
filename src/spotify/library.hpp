#ifndef LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#define LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "abstract.hpp"
#include "cache.hpp"
#include "utils.hpp"

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
protected:
    template<typename T>
    void request_paginated_data(const string &request_url, T &data, const string &data_key = "");
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

template<typename T>
void LibraryCache::request_paginated_data(const string &request_url, T &result, const string &data_key)
{
    // TODO: unfinished, ugly
    json url(request_url);

    size_t entries_received = 0;
    auto ss = config::ps_info.SaveScreen(0, 0, -1, -1);
    wstring message = L"Loading\nRequesting data...";

    do
    {
        config::ps_info.Message(&MainGuid,&FarMessageGuid,
            FMSG_ALLINONE,
            L"",
            (const wchar_t * const *)message.c_str(),
            0,0);

        auto r = api->get(url);
        if (utils::http::is_success(r->status))
        {
            json data = json::parse(r->body);
            if (!data_key.empty())
                data = data.at(data_key);

                url = data["next"];

            const auto &entries = data["items"].get<T>();
            result.insert(result.end(), entries.begin(), entries.end());
            
            entries_received += entries.size();
            message = std::format(L"Loading\nEntries received {}/{}",
                entries_received, data["total"].get<int>());
        }
    }
    while (!url.is_null());

    config::ps_info.RestoreScreen(ss);
}

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7