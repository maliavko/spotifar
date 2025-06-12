#include "collection.hpp"

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
    saved_tracks_collection(api_interface *api, collection *collection):
        saved_tracks_t(api->get_ptr(), "/v1/me/tracks"),
        collection(collection)
        {}
    ~saved_tracks_collection() { collection = nullptr; }

    bool fetch_items(api_weak_ptr_t api_proxy, bool only_cached, bool notify_watchers = true, size_t pages_to_request = 0) override
    {
        if (saved_tracks_t::fetch_items(api_proxy, only_cached, notify_watchers, pages_to_request))
        {
            item_ids_t ids;
            std::transform(cbegin(), cend(), std::back_inserter(ids), [](const auto &t) { return t.id; });

            collection->tracks.update_saved_items(ids, true);
            return true;
        }
        return false;
    }
private:
    collection *collection;
};

/// @brief A custom saved-tracks-collection specialization, to update collection cache,
/// when the fetching is finished.
class saved_albums_collection: public saved_albums_t
{
public:
    saved_albums_collection(api_interface *api, collection *collection):
        saved_albums_t(api->get_ptr(), "/v1/me/albums"),
        collection(collection)
        {}
    ~saved_albums_collection() { collection = nullptr; }

    bool fetch_items(api_weak_ptr_t api_proxy, bool only_cached, bool notify_watchers = true, size_t pages_to_request = 0) override
    {
        if (saved_albums_t::fetch_items(api_proxy, only_cached, notify_watchers, pages_to_request))
        {
            item_ids_t ids;
            std::transform(cbegin(), cend(), std::back_inserter(ids), [](const auto &t) { return t.id; });

            collection->albums.update_saved_items(ids, true);
            return true;
        }
        return false;
    }
private:
    collection *collection;
};

//-------------------------------------------------------------------------------------------------------------------
void from_json(const json::Value &j, saved_items_t &v)
{
    from_json(j["tracks"], v.tracks);
    from_json(j["albums"], v.albums);
}

void to_json(json::Value &result, const saved_items_t &v, json::Allocator &allocator)
{
    result = json::Value(json::kObjectType);

    json::Value tracks;
    to_json(tracks, v.tracks, allocator);

    json::Value albums;
    to_json(albums, v.albums, allocator);

    result.AddMember("tracks", json::Value(tracks, allocator), allocator);
    result.AddMember("albums", json::Value(albums, allocator), allocator);
}


static std::deque<bool> check_saved_items(api_interface *api, const string &endpoint_url, const item_ids_t &ids)
{
    // note: bloody hell, damn vector specialization with bools is not a real vector,
    // it cannot return ref to bools, so deque is used instead

    // note: player visual style methods are being called extremely often, the 'like` button,
    // which represents a state of a track being part of saved collection as well. That's why
    // the response here is cached for a session
    auto requester = several_items_requester<bool, -1, utils::clock_t::duration, std::deque<bool>>(
        endpoint_url, ids, BATCH_SIZE);

    if (requester.execute(api->get_ptr()))
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

        dispatch_event(ids);
        return true;
    }
    else
    {
        log::api->error("Failed to get saved status for {} tracks", ids.size());
    }
    return false;
}

void saved_items_cache_t::update_saved_items(const item_ids_t &ids, bool status)
{
    auto accessor = data_accessor();
    auto &container = get_container(accessor.data);

    item_ids_t changed_ids{};

    for (const auto &id: ids)
    {
        if (container.contains(id) && container.at(id) == status)
            continue;
            
        container.insert_or_assign(id, status);
        changed_ids.push_back(id);
    }
    
    if (!changed_ids.empty())
        dispatch_event(changed_ids);
}

bool saved_items_cache_t::is_item_saved(const item_id_t &item_id, bool force_sync)
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

void tracks_items_cache_t::dispatch_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_saved_tracks_changed, ids);
}

statuses_container_t& albums_items_cache_t::get_container(collection_base_t::data_t &data)
{
    return data.albums;
}

std::deque<bool> albums_items_cache_t::check_saved_items(api_interface *api, const item_ids_t &ids)
{
    return spotify::check_saved_items(api, "/v1/me/albums/contains", ids);
}

void albums_items_cache_t::dispatch_event(const item_ids_t &ids)
{
    utils::far3::synchro_tasks::dispatch_event(
        &collection_observer::on_saved_albums_changed, ids);
}


//-------------------------------------------------------------------------------------------------------------------
collection::collection(api_interface *api):
    json_cache(), api_proxy(api),
    tracks(api, [this] { return lock_data(); }),
    albums(api, [this] { return lock_data(); })
{
}

collection::~collection()
{
    api_proxy = nullptr;
}

bool collection::is_track_saved(const item_id_t &track_id, bool force_sync)
{
    return tracks.is_item_saved(track_id, force_sync);
}

bool collection::save_tracks(const item_ids_t &ids)
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

bool collection::remove_saved_tracks(const item_ids_t &ids)
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

saved_tracks_ptr collection::get_saved_tracks()
{
    return saved_tracks_ptr(new saved_tracks_collection(api_proxy, this));
}

saved_albums_ptr collection::get_saved_albums()
{
    return saved_albums_ptr(new saved_albums_collection(api_proxy, this));
}

bool collection::is_album_saved(const item_id_t &album_id, bool force_sync)
{
    return albums.is_item_saved(album_id, force_sync);
}

bool collection::save_albums(const item_ids_t &ids)
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

bool collection::remove_saved_albums(const item_ids_t &ids)
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

bool collection::is_active() const
{
    return api_proxy->is_authenticated();
}

clock_t::duration collection::get_sync_interval() const
{
    return 1500ms;
}

bool collection::request_data(data_t &data)
{
    data = get();

    tracks.resync(data.tracks);
    albums.resync(data.albums);

    return true;
}
    
} // namespace spotify
} // namespace spotifar