#include "library.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;
namespace http = utils::http;
namespace phs = std::placeholders;
using utils::far3::synchro_tasks::dispatch_event;

static const size_t BATCH_SIZE = 50;

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


std::deque<bool> check_saved_items(api_interface *api, const string &endpoint_url, const item_ids_t &ids)
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

        // dispatching an event to notify observers about the new saved status
        dispatch_event(&collection_observer::on_saved_tracks_changed, ids);
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

    for (const auto &id: ids)
        container.insert_or_assign(id, status);
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
statuses_container_t& tracks_items_cache_t::get_container(library_base_t::data_t &data)
{
    return data.tracks;
}

std::deque<bool> tracks_items_cache_t::check_saved_items(api_interface *api, const item_ids_t &ids)
{
    return spotify::check_saved_items(api, "/v1/me/tracks/contains", ids);
}

statuses_container_t& albums_items_cache_t::get_container(library_base_t::data_t &data)
{
    return data.tracks;
}

std::deque<bool> albums_items_cache_t::check_saved_items(api_interface *api, const item_ids_t &ids)
{
    return spotify::check_saved_items(api, "/v1/me/albums/contains", ids);
}


//-------------------------------------------------------------------------------------------------------------------
library::library(api_interface *api):
    json_cache(), api_proxy(api),
    tracks(api, [this] { return lock_data(); }),
    albums(api, [this] { return lock_data(); })
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

    dispatch_event(&collection_observer::on_saved_tracks_changed, ids);

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

    dispatch_event(&collection_observer::on_saved_tracks_changed, ids);
    
    return true;
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

    dispatch_event(&collection_observer::on_saved_albums_changed, ids);

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

    dispatch_event(&collection_observer::on_saved_albums_changed, ids);
    
    return true;
}

bool library::is_active() const
{
    return api_proxy->is_authenticated();
}

clock_t::duration library::get_sync_interval() const
{
    return 1500ms;
}

bool library::request_data(data_t &data)
{
    data = get();

    tracks.resync(data.tracks);
    albums.resync(data.albums);

    return true;
}
    
} // namespace spotify
} // namespace spotifar