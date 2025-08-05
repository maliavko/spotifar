 #include "library.hpp"
 #include "requesters.hpp"
 #include "observer_protocols.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;
namespace http = utils::http;
namespace phs = std::placeholders;

static const size_t BATCH_SIZE = 50;

/// @brief A custom saved-tracks-collection specialization, to update collection cache,
/// when the fetching is finished.
class saved_tracks_collection: public saved_tracks_t
{
public:
    /// @param collection A collection to be updated pointer
    saved_tracks_collection(api_interface *api, library *library):
        saved_tracks_t(api->get_ptr(), "/v1/me/tracks"),
        library(library)
        {}
    
    ~saved_tracks_collection() { library = nullptr; }

    bool fetch_items(api_weak_ptr_t api_proxy, bool only_cached, bool silent = false, size_t pages_to_request = 0) override
    {
        if (saved_tracks_t::fetch_items(api_proxy, only_cached, silent, pages_to_request))
        {
            item_ids_t ids;
            std::transform(cbegin(), cend(), std::back_inserter(ids), [](const auto &t) { return t.id; });

            library->tracks.update_saved_items(ids, true, pages_to_request == 0);
            return true;
        }
        return false;
    }
private:
    library *library;
};

/// @brief A custom saved-albums-collection specialization, to update collection cache,
/// when the fetching is finished. 
class saved_albums_collection: public saved_albums_t
{
public:
    /// @param collection A collection to be updated pointer
    saved_albums_collection(api_interface *api, library *library):
        saved_albums_t(api->get_ptr(), "/v1/me/albums"),
        library(library)
        {}
    ~saved_albums_collection() { library = nullptr; }

    bool fetch_items(api_weak_ptr_t api_proxy, bool only_cached, bool silent = false, size_t pages_to_request = 0) override
    {
        if (saved_albums_t::fetch_items(api_proxy, only_cached, silent, pages_to_request))
        {
            item_ids_t ids;
            std::transform(cbegin(), cend(), std::back_inserter(ids), [](const auto &t) { return t.id; });

            library->albums.update_saved_items(ids, true, pages_to_request == 0);
            return true;
        }
        return false;
    }
private:
    library *library;
};

/// @brief A custom saved-artists-collection specialization, to update collection cache,
/// when the fetching is finished. 
class followed_artists_collection: public followed_artists_t
{
public:
    /// @param collection A collection to be updated pointer
    followed_artists_collection(api_interface *api, library *library):
        followed_artists_t(api->get_ptr(), "/v1/me/following", {{ "type", "artist" }}, "artists"),
        library(library)
        {}
    ~followed_artists_collection() { library = nullptr; }

    bool fetch_items(api_weak_ptr_t api_proxy, bool only_cached, bool silent = false, size_t pages_to_request = 0) override
    {
        if (followed_artists_t::fetch_items(api_proxy, only_cached, silent, pages_to_request))
        {
            item_ids_t ids;
            std::transform(cbegin(), cend(), std::back_inserter(ids), [](const auto &t) { return t.id; });
            
            library->artists.update_saved_items(ids, true, pages_to_request == 0);

            return true;
        }
        return false;
    }
private:
    library *library;
};

//-------------------------------------------------------------------------------------------------------------------
void from_json(const json::Value &j, saved_items_t &v)
{
    if (!j["tracks"].IsNull())
        from_json(j["tracks"], v.tracks);

    if (!j["albums"].IsNull())
        from_json(j["albums"], v.albums);

    if (!j["artists"].IsNull())
        from_json(j["artists"], v.artists);
}

void to_json(json::Value &result, const saved_items_t &v, json::Allocator &allocator)
{
    result = json::Value(json::kObjectType);

    json::Value tracks;
    to_json(tracks, v.tracks, allocator);

    json::Value albums;
    to_json(albums, v.albums, allocator);

    json::Value artists;
    to_json(artists, v.artists, allocator);

    result.AddMember("tracks", json::Value(tracks, allocator), allocator);
    result.AddMember("albums", json::Value(albums, allocator), allocator);
    result.AddMember("artists", json::Value(artists, allocator), allocator);
}


static std::deque<bool> check_saved_items(api_interface *api, const string &endpoint_url, const item_ids_t &ids)
{
    // NOTE: bloody hell, damn vector specialization with bools is not a real vector,
    // it cannot return ref to bools, so deque is used instead

    // NOTE: player visual style methods are being called extremely often, the 'like` button,
    // which represents a state of a track being part of saved collection as well. That's why
    // the response here is cached for a session
    auto requester = several_items_requester<bool, -1, utils::clock_t::duration, std::deque<bool>>(
        endpoint_url, ids, BATCH_SIZE);

    if (requester.execute(api->get_ptr(), false, false))
        return requester.get();
    
    // perhaps at some point it'd be good to add an error propagation here
    // to show to the user
    return {};
}


//-------------------------------------------------------------------------------------------------------------------
bool saved_items_cache_t::resync(statuses_container_t &data)
{
    if (ids_to_process.empty()) return false;

    item_ids_t ids;

    {
        std::lock_guard<std::mutex> lock(ids_access_guard);
        if (ids_to_process.size() > BATCH_SIZE)
        {
            ids.assign(ids_to_process.begin(), ids_to_process.begin() + BATCH_SIZE);
            ids_to_process.erase(ids_to_process.begin(), ids_to_process.begin() + BATCH_SIZE);
        }
        else
            ids = std::move(ids_to_process);
    }

    if (auto result = check_saved_items(api_proxy, ids); result.size() == ids.size())
    {
        for (size_t i = 0; i < ids.size(); ++i)
            data.insert_or_assign(ids[i], result[i]);

        // potential problem, as until the method returns 'true`, the cache
        // does not save `data` into its container; if some subscriber, revceiving
        // the event decides to access cache, it is outdated
        statuses_received_event(ids);
        return true;
    }
    else
    {
        log::api->error("Failed to get saved status for {} items", ids.size());
    }
    return false;
}

void saved_items_cache_t::update_saved_items(const item_ids_t &ids, bool status, bool full_resync)
{
    item_ids_t changed_ids, received_ids;
    {
        auto accessor = data_accessor();
        auto &container = get_container(accessor.data);

        for (const auto &id: ids)
        {
            if (const auto it = container.find(id); it != container.end())
            {
                if (it->second != status)
                    changed_ids.push_back(id);
            }
            else
            {
                received_ids.push_back(id);
            }
            container[id] = status;
        }

        // the flag comes from the saved collections fetch method, specifying
        // that the given `ids` are the full set of saved items
        if (full_resync)
        {
            // ... in that case we are checking for the ones, removed from collection
            const std::unordered_set<item_id_t> unique_ids(ids.begin(), ids.end());
            for (auto &[id, value]: container)
                // ... does not present in the incoming ids and was true in the container
                if (!unique_ids.contains(id) && value == true)
                    changed_ids.push_back(id);
        }
    }
    
    if (!changed_ids.empty())
        statuses_changed_event(changed_ids);
    
    if (!received_ids.empty())
        statuses_received_event(received_ids);
}

bool saved_items_cache_t::is_item_saved(const item_id_t &item_id, bool force_sync)
{
    {
        auto accessor = data_accessor();
        auto &container = get_container(accessor.data);

        const auto it = container.find(item_id);
        if (it != container.end())
            return it->second;

        if (force_sync)
        {
            if (auto res = check_saved_items(api_proxy, { item_id }); res.size() == 1)
            {
                container.insert_or_assign(item_id, res[0]);
                return res[0];
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(ids_access_guard);
        if (std::find(ids_to_process.begin(), ids_to_process.end(), item_id) == ids_to_process.end())
            ids_to_process.push_back(item_id);
    }

    return false;
}


//-------------------------------------------------------------------------------------------------------------------
statuses_container_t& tracks_items_cache_t::get_container(collection_base_t::data_t &data)
{
    return data.tracks;
}

std::deque<bool> tracks_items_cache_t::check_saved_items(api_interface *api, const item_ids_t &ids)
{
    return spotify::check_saved_items(api, "/v1/me/tracks/contains", ids);
}

void tracks_items_cache_t::statuses_received_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_tracks_statuses_received, ids);
}

void tracks_items_cache_t::statuses_changed_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_tracks_statuses_changed, ids);
}


statuses_container_t& albums_items_cache_t::get_container(collection_base_t::data_t &data)
{
    return data.albums;
}

std::deque<bool> albums_items_cache_t::check_saved_items(api_interface *api, const item_ids_t &ids)
{
    return spotify::check_saved_items(api, "/v1/me/albums/contains", ids);
}

void albums_items_cache_t::statuses_received_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_albums_statuses_received, ids);
}

void albums_items_cache_t::statuses_changed_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_albums_statuses_changed, ids);
}


statuses_container_t& artists_items_cache_t::get_container(collection_base_t::data_t &data)
{
    return data.artists;
}

std::deque<bool> artists_items_cache_t::check_saved_items(api_interface *api, const item_ids_t &ids)
{
    // possible types: "artist" and "user"
    const auto url = httplib::append_query_params("/v1/me/following/contains", {{ "type", "artist" }});

    return spotify::check_saved_items(api, url, ids);
}

void artists_items_cache_t::statuses_received_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_artists_statuses_received, ids);
}

void artists_items_cache_t::statuses_changed_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_artists_statuses_changed, ids);
}


//-------------------------------------------------------------------------------------------------------------------
library::library(api_interface *api):
    json_cache(), api_proxy(api),
    tracks(api, [this] { return lock_data(); }),
    albums(api, [this] { return lock_data(); }),
    artists(api, [this] { return lock_data(); })
{
}

library::~library()
{
    api_proxy = nullptr;
}

bool library::is_track_saved(const item_id_t &track_id, bool force_sync)
{
    return tracks.is_item_saved(track_id, force_sync);
}

bool library::save_tracks(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto requester = put_requester("/v1/me/tracks", body.str());
    if (!requester.execute(api_proxy->get_ptr()))
    {
        playback_cmd_error(http::get_status_message(requester.get_response()));
        return false;
    }
    
    // updating tracks cache with the new saved tracks ids
    tracks.update_saved_items(ids, true);

    return true;
}

bool library::remove_saved_tracks(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto requester = del_requester("/v1/me/tracks", body.str());
    if (!requester.execute(api_proxy->get_ptr()))
    {
        playback_cmd_error(http::get_status_message(requester.get_response()));
        return false;
    }
    
    // updating tracks cache with the new saved tracks ids
    tracks.update_saved_items(ids, false);
    
    return true;
}

saved_tracks_ptr library::get_saved_tracks()
{
    return saved_tracks_ptr(new saved_tracks_collection(api_proxy, this));
}

bool library::is_album_saved(const item_id_t &album_id, bool force_sync)
{
    return albums.is_item_saved(album_id, force_sync);
}

bool library::save_albums(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto requester = put_requester("/v1/me/albums", body.str());
    if (!requester.execute(api_proxy->get_ptr()))
    {
        playback_cmd_error(http::get_status_message(requester.get_response()));
        return false;
    }
    
    // updating albums cache with the new saved albums ids
    albums.update_saved_items(ids, true);

    return true;
}

bool library::remove_saved_albums(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    auto requester = del_requester("/v1/me/albums", body.str());
    if (!requester.execute(api_proxy->get_ptr()))
    {
        playback_cmd_error(http::get_status_message(requester.get_response()));
        return false;
    }
    
    // updating albums cache with the new saved albums ids
    albums.update_saved_items(ids, false);
    
    return true;
}

saved_albums_ptr library::get_saved_albums()
{
    return saved_albums_ptr(new saved_albums_collection(api_proxy, this));
}

bool library::is_artist_followed(const item_id_t &artist_id, bool force_sync)
{
    return artists.is_item_saved(artist_id, force_sync);
}

bool library::follow_artists(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    // possible types: "artist" and "user"
    const auto url = httplib::append_query_params("/v1/me/following", {{ "type", "artist" }});

    auto requester = put_requester(url, body.str());
    if (!requester.execute(api_proxy->get_ptr()))
    {
        playback_cmd_error(http::get_status_message(requester.get_response()));
        return false;
    }
    
    // updating artists cache with the new saved artists ids
    artists.update_saved_items(ids, true);

    return true;
}

bool library::unfollow_artists(const item_ids_t &ids)
{
    http::json_body_builder body;

    body.object([&]
    {
        body.insert("ids", ids);
    });

    // possible types: "artist" and "user"
    const auto url = httplib::append_query_params("/v1/me/following", {{ "type", "artist" }});

    auto requester = del_requester(url, body.str());
    if (!requester.execute(api_proxy->get_ptr()))
    {
        playback_cmd_error(http::get_status_message(requester.get_response()));
        return false;
    }
    
    // updating artists cache with the new saved artists ids
    artists.update_saved_items(ids, false);
    
    return true;
}

followed_artists_ptr library::get_followed_artists()
{
    return followed_artists_ptr(new followed_artists_collection(api_proxy, this));
}

bool library::is_active() const
{
    bool is_authenticated = api_proxy->get_auth_cache()->is_authenticated();
    bool is_rate_limited = api_proxy->is_endpoint_rate_limited("me");
    
    return is_authenticated && !is_rate_limited;
}

clock_t::duration library::get_sync_interval() const
{
    return 1500ms;
}

bool library::request_data(data_t &data)
{
    data = get();

    if (tracks.resync(data.tracks) || albums.resync(data.albums) || artists.resync(data.artists))
        return true;

    return false;
}
    
} // namespace spotify
} // namespace spotifar