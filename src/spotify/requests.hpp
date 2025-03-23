#ifndef REQUESTS_HPP_40324BB1_0A71_4030_8792_1F98850B6550
#define REQUESTS_HPP_40324BB1_0A71_4030_8792_1F98850B6550
#pragma once

#include "abstract.hpp"

namespace spotifar { namespace spotify {

static const size_t MAX_LIMIT = 50;

/// @brief A class-helper for the API requests, incapsulates some simple logic for 
/// common requests operations. Requests, validates response, parses result,
/// returns mapped data.
///
/// @tparam T a resulting object type, json item
template<class T>
struct api_requester
{
    typedef typename T value_t;
    
    T result; // result holder
    json body;
    string url; // initial request url string
    string data_field; // some responses have nested data under `data_field` key name
    Result response; // request httplib::Response

    /// @param request_url initial request url
    /// @param params request params object
    /// @param data_field some responses have nested data under `data_field` key name
    api_requester(const string &request_url, httplib::Params params = {}, const string &data_field = ""):
        data_field(data_field)
    {
        url = httplib::append_query_params(request_url, params);
    }
    
    const string &get_url() const { return url; }
    
    /// @brief Returns a reference to the requested data.
    /// @note The result is valid only after a successful response
    const T& get() const { return result; }

    const json& get_data() { return body; }

    /// @brief Whether the performed response is succeeded
    bool is_success() const { return utils::http::is_success(response->status); }

    /// @brief Executing an API request
    /// @return a flag, whether the request succeeded or not
    bool operator()(api_abstract *api)
    {
        response = api->get(url, utils::http::session);
        if (is_success())
        {
            body = json::parse(response->body);

            // the needed data is nested, we rebind references deeper
            if (!data_field.empty())
                on_success(body.at(data_field));
            else
                on_success(body);

            return true;
        }
        return false;
    }

    /// @brief The method is called right after the valid response is received and
    /// data is parsed correctly. Base method also reads a resulting value in this method
    virtual void on_success(const json &data)
    {
        data.get_to(result);
    }
};

/// @brief A class-helpers to perform the API requests, for the paginated data.
/// @tparam T a resulting data type
template<class T>
struct api_collection_requester: public api_requester<T>
{
    using api_requester<T>::api_requester; // base ctor

    /// @brief Returns the total amount of entries in general
    /// @note The result is valid only after a successful response
    size_t get_total() const { return total; }

    /// @brief Can be further iterated or not 
    bool has_more() const { return !this->url.empty(); }
    
    virtual void on_success(const json &data)
    {
        data["items"].get_to(this->result);
        this->total = data.value("total", 0);

        auto next = data["next"];
        this->url = !next.is_null() ? next.get<string>() : "";
    }

    /// @brief Iterating items page by page of a given `limit` size
    std::generator<T> fetch_by_pages(api_abstract *api)
    {
        // auto ss = config::ps_info.SaveScreen(0,0,-1,-1);

        // size_t entries_received = 0;
        // wstring message = L"\nRequesting data...";

        do
        {
            // config::ps_info.Message(&MainGuid,&FarMessageGuid,
            //     FMSG_ALLINONE,
            //     L"",
            //     (const wchar_t * const *)message.c_str(),
            //     0,0);

            if ((*this)(api))
                co_yield this->result;
            
            // entries_received += this->result.size();
            // message = std::format(L"\nEntries received {} / {}",
            //     entries_received, this->get_total());
        }
        while (this->has_more());
        
        // config::ps_info.RestoreScreen(ss);
    }

private:
    size_t total = 0;
};

/// @brief A class-helpers to request several items from Spotify. As their
/// API allows requesting with a limited number of items, the requester implements
/// an interface to get data by chunks.
/// @tparam T the tyope of the data returned, iterable
template<class T>
struct api_several_items_requester
{
    typedef typename T value_t;
    
    /// @param chunk_size a max size of a data chunk to request
    /// @param data_field some responses have nested data under `data_field` key name
    api_several_items_requester(
        const string &url, const std::vector<string> ids, size_t chunk_size,
        const string &data_field = ""
    ):
        url(url), chunk_size(chunk_size), ids(ids), data_field(data_field)
        {}

    std::generator<T> fetch_by_chunks(api_abstract *api)
    {
        auto chunk_begin = ids.begin();
        auto chunk_end = ids.begin();

        do
        {
            if (std::distance(chunk_end, ids.end()) <= chunk_size)
                chunk_end = ids.end(); // the number of ids is less than the max chunk size
            else
                std::advance(chunk_end, chunk_size); // ..or just advance iterator further

            api_requester<T> requester(url, {
                { "ids", utils::string_join(std::vector<string>(chunk_begin, chunk_end), ",") },
            }, data_field);
            
            if (requester(api))
                co_yield requester.get();

            chunk_begin = chunk_end;
        }
        while (std::distance(chunk_begin, ids.end()) > 0);
    }
private:
    ptrdiff_t chunk_size;
    std::vector<string> ids;
    string data_field;
    string url;
};

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


/// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-artists
struct artists_requester: public api_several_items_requester<artists_t>
{
    artists_requester(const std::vector<string> &ids):
        api_several_items_requester("/v1/artists", ids, 50, "artists")
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

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-multiple-albums
struct albums_requester: public api_several_items_requester<albums_t>
{
    albums_requester(const std::vector<string> &ids):
        api_several_items_requester("/v1/albums", ids, 20, "albums")
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

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-recently-played
struct recently_played_requester: public api_collection_requester<history_items_t>
{
    recently_played_requester(std::int64_t timestamp_after, size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/me/player/recently-played", {
            { "limit", std::to_string(limit) },
            { "after", std::to_string(timestamp_after) },
        })
        {}
};

} // namespace spotify
} // namespace spotifar

#endif // REQUESTS_HPP_40324BB1_0A71_4030_8792_1F98850B6550