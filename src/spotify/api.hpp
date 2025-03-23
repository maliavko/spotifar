#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "abstract.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "auth.hpp"
#include "history.hpp"

namespace spotifar { namespace spotify {

template<class T>
class item_requester
{
public:
    item_requester(const string &url, httplib::Params params = {},
                   const string &fieldname = ""):
        url(httplib::append_query_params(url, params)),
        fieldname(fieldname)
        {}
    
    const T& get() const { return result; }

    const string& get_url() const { return url; }

    bool operator()(api_abstract *api)
    {
        auto res = api->get(url, utils::http::session);
        if (utils::http::is_success(res->status))
        {
            auto body = json::parse(res->body);

            if (!fieldname.empty())
                body = body.at(fieldname);

            on_read_result(body, result);

            return true;
        }
        return false;
    }
protected:
    virtual void on_read_result(const json &body, T &result)
    {
        body.get_to(result);
    }
protected:
    string fieldname;
    string url;
    T result;
};



template<class T>
class collection_requester: public item_requester<T>
{
public:
    using item_requester<T>::item_requester;

    const string get_next_url() const { return !next.is_null() ? next.get<string>() : ""; }

    size_t get_total() const { return total; }
protected:
    void on_read_result(const json &body, T &result) override
    {
        item_requester<T>::on_read_result(body.at("items"), result);

        total = body.value("total", 0);
        next = body.at("next");
    }
private:
    size_t total = 0;
    json next = nullptr;
};


template<class T>
class collection_abstract: public std::vector<T>
{
public:
    typedef std::vector<T> container_t;
    typedef collection_requester<container_t> requester_t;
    typedef std::shared_ptr<requester_t> requester_ptr;
public:
    collection_abstract(const string &request_url, httplib::Params params = {},
                        const string &fieldname = ""):
        url(request_url), params(params), fieldname(fieldname)
    {
        this->params.insert(std::pair{ "limit", "50" });
    }

    size_t get_total(api_abstract *api) const
    {
        if (is_populated)
            return this->size();

        auto requester = get_default_requester();
        if ((*requester)(api))
            return requester->get_total();
        
        return 0LL;
    }

    size_t peek_total(api_abstract *api) const
    {
        auto requester = get_default_requester();
        if (api->is_request_cached(requester->get_url()))
            if ((*requester)(api))
                return requester->get_total();
        
        return 0LL;
    }

    bool fetch(api_abstract *api)
    {
        if (!fetch_all(api))
            return false;

        is_populated = true;

        return true;
    }
protected:
    virtual requester_ptr get_default_requester() const = 0;

    virtual bool fetch_all(api_abstract *api) = 0;
protected:
    string url;
    string fieldname;
    httplib::Params params;
    bool is_populated = false;
};



template<class T>
class sync_collection: public collection_abstract<T>
{
public:
    using collection_abstract<T>::requester_t;
    using collection_abstract<T>::requester_ptr;
    using collection_abstract<T>::collection_abstract;
protected:
    bool fetch_all(api_abstract *api) override
    {
        auto requester = get_default_requester();

        while (requester != nullptr)
        {
            if (!(*requester)(api))
                return false;

            if (this->capacity() != requester->get_total())
                this->reserve(requester->get_total());
                
            auto &entries = requester->get();
            this->insert(this->end(), entries.begin(), entries.end());
    
            const auto &next_url = requester->get_next_url();
            if (!next_url.empty())
                requester = requester_ptr(new requester_t(next_url, {}, this->fieldname));
        }

        return true;
    }

    requester_ptr get_default_requester() const override
    {
        return requester_ptr(new requester_t(this->url, this->params, this->fieldname));
    }
};



template<class T>
class async_collection: public collection_abstract<T>
{
public:
    using collection_abstract<T>::requester_t;
    using collection_abstract<T>::requester_ptr;
    using collection_abstract<T>::collection_abstract;
protected:
    bool fetch_all(api_abstract *api) override
    {
        auto requester = make_requester(0);

        size_t total = 0;
        if ((*requester)(api))
            total = requester->get_total();

        if (total == 0) return false;

        if (this->capacity() != total)
            this->resize(total);

        size_t start = 1ULL, end = total / 50LL;
        if (total - end * 50 > 0)
            end += 1;

        const auto &entries = requester->get();
        this->insert(this->end(), entries.begin(), entries.end());

        BS::multi_future<void> sequence_future = api->get_pool().submit_sequence(start, end,
            [this, api](const size_t idx)
            {
                auto requester = make_requester(idx * 50);
                if ((*requester)(api))
                {
                    const auto &entries = requester->get();
                    std::copy(entries.begin(), entries.end(), this->begin() + idx * 50);
                }
            });
        sequence_future.wait();

        return true;
    }

    requester_ptr get_default_requester() const override
    {
        return make_requester(0);
    }

    requester_ptr make_requester(size_t offset) const
    {
        auto updated_params = this->params;
        updated_params.insert(std::pair{ "offset", std::to_string(offset) });

        return requester_ptr(new requester_t(this->url, updated_params, this->fieldname));
    }
};


class api:
    public api_abstract,
    public auth_observer
{
public:
    api();
    ~api();

    bool start();
    void shutdown();
    void tick();

    void clear_http_cache();
    bool is_request_cached(const string &url) const;

    bool is_authenticated() const { return auth->is_authenticated(); }
    
    // library api interface
    auto get_play_history() -> const history_items_t& { return history->get(); }
    auto get_available_devices() -> const devices_t& { return devices->get(); }
    auto get_playback_state() -> const playback_state_t& { return playback->get(); }
    auto get_followed_artists() -> const artists_t&;
    auto get_artist(const string &artist_id) -> artist_t;
    auto get_artists(const std::vector<string> &ids) -> const artists_t&;
    auto get_artist_albums(const string &artist_id) -> const simplified_albums_t&;
    auto get_saved_albums() -> const saved_albums_t&;
    auto get_new_releases() -> const simplified_albums_t&;
    auto get_artist_top_tracks(const string &artist_id) -> tracks_t;
    auto get_album(const string &album_id) -> album_t;
    auto get_albums(const std::vector<string> &ids) -> albums_t;
    auto get_album_tracks(const string &album_id) -> const simplified_tracks_t&;
    auto get_playlist(const string &playlist_id) -> playlist_t;
    auto get_playlists() -> const simplified_playlists_t&;
    auto get_playlist_tracks(const string &playlist_id) -> const saved_tracks_t&;
    auto check_saved_track(const string &track_id) -> bool;
    auto check_saved_tracks(const std::vector<string> &ids) -> std::vector<bool>;
    auto save_tracks(const std::vector<string> &ids) -> bool;
    auto remove_saved_tracks(const std::vector<string> &ids) -> bool;
    auto get_playing_queue() -> playing_queue_t;
    auto get_recently_played(std::int64_t after) -> const history_items_t&;

    // playback api interface
    void start_playback(const string &context_uri, const string &track_uri = "",
        int position_ms = 0, const string &device_id = "");
    void start_playback(const std::vector<string> &uris, const string &device_id = "");
    void start_playback(const simplified_album_t &album, const simplified_track_t &track);
    void start_playback(const simplified_playlist_t &playlist, const simplified_track_t &track);
    void resume_playback(const string &device_id = "");
    void toggle_playback(const string &device_id = "");
    void pause_playback(const string &device_id = "");
    void skip_to_next(const string &device_id = "");
    void skip_to_previous(const string &device_id = "");
    void seek_to_position(int position_ms, const string &device_id = "");
    void toggle_shuffle(bool is_on, const string &device_id = "");
    void toggle_shuffle_plus(bool is_on);
    void set_repeat_state(const string &mode, const string &device_id = "");
    void set_playback_volume(int volume_percent, const string &device_id = "");
    void transfer_playback(const string &device_id, bool start_playing = false);

protected:
    void start_playback(const json &body, const string &device_id);
    
    // the main interface for raw http requests
    Result get(const string &url, utils::clock_t::duration cache_for = {});
    Result put(const string &url, const json &body = {});
    Result del(const string &url, const json &body = {});
    Result post(const string &url, const json &body = {});
    
    BS::thread_pool& get_pool() override { return pool; };

    std::shared_ptr<httplib::Client> get_client() const;

    /// @brief a helpers function for getting one item from API
    template<class R, typename... ArgumentsTypes>
    auto get_item(ArgumentsTypes... args) -> typename R::value_t;

    /// @brief a helpers function for getting one item from API
    template<class R, typename... ArgumentsTypes>
    auto get_several_items(ArgumentsTypes... args) -> const typename R::value_t&;

    /// @brief a helpers function for getting collection items from API
    template<class R, typename... ArgumentsTypes>
    auto get_items_collection(ArgumentsTypes... args) -> const typename R::value_t&;
    
    void on_auth_status_changed(const auth_t &auth); // auth status listener

private:
    BS::thread_pool pool;
    httplib::Client client;
    http_cache api_responses_cache;

    // caches
    std::unique_ptr<playback_cache> playback;
    std::unique_ptr<devices_cache> devices;
    std::unique_ptr<auth_cache> auth;
    std::unique_ptr<play_history> history;

    std::vector<cached_data_abstract*> caches;
};

template<class R, typename... ArgumentsTypes>
auto api::get_item(ArgumentsTypes... args) -> typename R::value_t
{
    auto requester = R(args...);
    if (requester(this))
        return requester.get();
    return {};
}

template<class R, typename... ArgumentsTypes>
auto api::get_several_items(ArgumentsTypes... args) -> const typename R::value_t&
{
    // keeping static container with data for being able to return a reference
    static typename R::value_t result;
    result.clear();

    // accumulating chunked results into one container
    auto requester = R(args...);
    for (const auto &entries: requester.fetch_by_chunks(this))
        result.insert(result.end(), entries.begin(), entries.end());

    return result;
}

template<class R, typename... ArgumentsTypes>
auto api::get_items_collection(ArgumentsTypes... args) -> const typename R::value_t&
{
    // keeping static container with data for being able to return a reference
    static typename R::value_t result;
    result.clear();

    // accumulating paged results into one container
    auto requester = R(args...);
    for (const auto &entries: requester.fetch_by_pages(this))
    {
        result.insert(result.end(), entries.begin(), entries.end());
        //break; // TODO: remove! just for speeding up the testing
    }

    return result;
}

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66