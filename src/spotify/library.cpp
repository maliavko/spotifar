#include "library.hpp"

namespace spotifar { namespace spotify {

namespace http = utils::http;

LibraryCache::LibraryCache(api_abstract *api):
    api(api)
{
}

LibraryCache::~LibraryCache()
{
    storages.clear();
}

void LibraryCache::read(settings_ctx &ctx)
{
    for (auto &s: storages)
        s->read(ctx);
}

void LibraryCache::write(settings_ctx &ctx)
{
    for (auto &s: storages)
        s->write(ctx);
}

void LibraryCache::clear(settings_ctx &ctx)
{
    for (auto &s: storages)
        s->clear(ctx);
}

void LibraryCache::resync(bool force)
{
    if (!api->is_authenticated())
        return;
}

artists_t LibraryCache::get_followed_artists()
{
    artists_t result;

    request_paginated_data(
        httplib::append_query_params("/v1/me/following", {
            { "type", "artist" },
            { "limit", std::to_string(50) },
        }),
        result,
        "artists"
    );

    return result;
}

artist LibraryCache::get_artist(const string &artist_id)
{
    auto r = api->get(std::format("/v1/artists/{}", artist_id));
    if (http::is_success(r->status))
        return json::parse(r->body).get<artist>();
    return artist();
}

albums_t LibraryCache::get_artist_albums(const string &artist_id)
{
    albums_t result;

    json request_url = httplib::append_query_params(
        std::format("/v1/artists/{}/albums", artist_id), {
            { "limit", "50" },
            { "include_groups", "album" }
        });
        
    do
    {
        auto r = api->get(request_url);
        if (http::is_success(r->status))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &albums = data["items"].get<albums_t>();
            result.insert(result.end(), albums.begin(), albums.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

tracks_t LibraryCache::get_artist_top_tracks(const string &artist_id)
{
    auto r = api->get(std::format("/v1/artists/{}/top-tracks", artist_id));

    if (http::is_success(r->status))
    {
        json data = json::parse(r->body);
        return data["tracks"].get<tracks_t>();
    }
    return {};
}
    
album LibraryCache::get_album(const string &album_id)
{
    auto r = api->get(std::format("/v1/albums/{}", album_id));
    if (http::is_success(r->status))
        return json::parse(r->body).get<album>();
    return album();
}

simplified_tracks_t LibraryCache::get_album_tracks(const string &album_id)
{
    simplified_tracks_t result;
    
    json request_url = httplib::append_query_params(
        std::format("/v1/albums/{}/tracks", album_id), {
            { "limit", std::to_string(50) },
        });
        
    do
    {
        auto r = api->get(request_url);
        if (http::is_success(r->status))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &tracks = data["items"].get<simplified_tracks_t>();
            result.insert(result.end(), tracks.begin(), tracks.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

playlist LibraryCache::get_playlist(const string &playlist_id)
{
    json request_url = httplib::append_query_params(
        std::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist::get_fields_filter() },
        });

    auto r = api->get(request_url);
    if (http::is_success(r->status))
        return json::parse(r->body).get<playlist>();
    return playlist();
}

simplified_playlists_t LibraryCache::get_playlists()
{
    simplified_playlists_t result;

    json request_url = httplib::append_query_params("/v1/me/playlists", {
        { "limit", "50" }
    });
    
    do
    {
        auto r = api->get(request_url);
        if (http::is_success(r->status))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &playlists = data["items"].get<simplified_playlists_t>();
            result.insert(result.end(), playlists.begin(), playlists.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

playlist_tracks_t LibraryCache::get_playlist_tracks(const string &playlist_id)
{
    static string fields = std::format("items({}),next,total", playlist_track::get_fields_filter());

    playlist_tracks_t result;

    request_paginated_data(
        httplib::append_query_params(
            std::format("/v1/playlists/{}/tracks", playlist_id), {
                { "limit", std::to_string(50) },
                { "additional_types", "track" },
                { "fields", fields },
            }),
        result
    );

    return result;
}

} // namespace spotify
} // namespace spotifar