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

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
struct saved_tracks_requester: public api_collection_requester<saved_tracks_t>
{
    saved_tracks_requester(size_t limit = MAX_LIMIT):
        api_collection_requester("/v1/me/tracks", {
            { "limit", std::to_string(limit) }
        })
        {}
};

/// @brief https://developer.spotify.com/documentation/web-api/reference/get-an-artists-top-tracks
struct artist_top_tracks_requester: public api_requester<tracks_t>
{
    artist_top_tracks_requester(const string &artist_id):
        api_requester(std::format("/v1/artists/{}/top-tracks", artist_id))
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