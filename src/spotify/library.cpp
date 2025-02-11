#include "library.hpp"
#include "utils.hpp"

namespace spotifar { namespace spotify {

using namespace utils;

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

    if (!is_initialized)
    {
        is_initialized = true;

        log::api->debug("Library initialization");

        json request_url = httplib::append_query_params("/v1/me/following", {
            { "type", "artist" },
            { "limit", std::to_string(50) },
        });

        do
        {
            if (auto r = api->get(request_url))
            {
                json data = json::parse(r->body)["artists"];
                request_url = data["next"];

                artists_list_t result;
                for (const auto &artist: data["items"].get<artists_list_t>())
                {
                    auto &ref = artists[artist.id] = artist;
                    followed_artists.push_back(&ref);
                }
            }
        }
        while (!request_url.is_null());
    }
}
    
const artist* LibraryCache::get_artist(const string &artist_id)
{
    // looking for the artist in cache
    auto it = artists.find(artist_id);
    if (it != artists.end())
        return &it->second;

    // otherwise requesting from the server
    if (auto r = api->get(std::format("/v1/artists/{}", artist_id)))
    {
        auto &ref = artists[artist_id] = json::parse(r->body).get<artist>();
        return &ref;
    }
    return nullptr;
}
    
const album* LibraryCache::get_album(const string &album_id)
{
    // looking for the album in cache
    auto it = albums.find(album_id);
    if (it != albums.end())
        return &it->second;

    // otherwise requesting from the server
    if (auto r = api->get(std::format("/v1/albums/{}", album_id)))
    {
        auto &ref = albums[album_id] = json::parse(r->body).get<album>();
        return &ref;
    }
    return nullptr;
}

const playlist* LibraryCache::get_playlist(const string &playlist_id)
{
    // looking for the album in cache
    // auto it = playlists.find(playlist_id);
    // if (it != playlists.end())
    //     return &it->second;

    json request_url = httplib::append_query_params(
        std::format("/v1/playlists/{}", playlist_id), {
            { "additional_types", "track" },
            { "fields", playlist::get_fields_filter() },
        });

    // otherwise requesting from the server
    if (auto r = api->get(request_url))
    {
        auto &ref = playlists[playlist_id] = json::parse(r->body).get<playlist>();
        return &ref;
    }
    return nullptr;
}

} // namespace spotify
} // namespace spotifar