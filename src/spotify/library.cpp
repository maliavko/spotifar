#include "library.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;
namespace http = utils::http;
using utils::far3::synchro_tasks::dispatch_event;

static const size_t batch_size = 50;

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

bool library::is_track_saved(const item_id_t &track_id, bool force_sync)
{
    auto &tracks = value.get().tracks;

    const auto it = tracks.find(track_id);
    if (it != tracks.end())
        return it->second;

    if (force_sync)
    {
        if (auto res = check_saved_tracks({ track_id }); res.size() == 1)
        {
            tracks.insert({ track_id, res[0] });
            return res[0];
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(data_access_guard);
        if (std::find(tracks_to_process.begin(), tracks_to_process.end(), track_id) == tracks_to_process.end())
            tracks_to_process.push_back(track_id);
    }

    return false;
}

std::deque<bool> library::check_saved_tracks(const item_ids_t &ids)
{
    // note: bloody hell, damn vector specialization with bools is not a real vector,
    // it cannot return ref to bools, so deque is used instead

    // note: player visual style methods are being called extremely often, the 'like` button,
    // which represents a state of a track being part of saved collection as well. That's why
    // the response here is cached for a session
    auto requester = several_items_requester<bool, -1, utils::clock_t::duration, std::deque<bool>>(
        "/v1/me/tracks/contains", ids, batch_size);

    if (requester.execute(api_proxy->get_ptr()))
        return requester.get();
    
    // perhaps at some point it'd be good to add an error propagation here
    // to show to the user
    return {};
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
    auto &tracks = value.get().tracks;
    for (const auto &id: ids)
        tracks.insert_or_assign(id, true);

    dispatch_event(&collection_observer::on_saved_tracks_status_received, ids);

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
    auto &tracks = value.get().tracks;
    for (const auto &id: ids)
        tracks.insert_or_assign(id, false);

    dispatch_event(&collection_observer::on_saved_tracks_status_received, ids);
    
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
    if (tracks_to_process.empty()) return false;

    if (api_proxy != nullptr)
    {
        item_ids_t ids;
        {
            std::lock_guard<std::mutex> lock(data_access_guard);
            if (tracks_to_process.size() > batch_size)
            {
                ids.assign(tracks_to_process.begin(), tracks_to_process.begin() + batch_size);
                tracks_to_process.erase(tracks_to_process.begin(), tracks_to_process.begin() + batch_size);
            }
            else
                ids = std::move(tracks_to_process);
        }

        if (auto result = check_saved_tracks(ids); result.size() == ids.size())
        {
            data = get();

            for (size_t i = 0; i < ids.size(); ++i)
                data.tracks.insert_or_assign(ids[i], result[i]);

            // dispatching an event to notify observers about the new saved status
            dispatch_event(&collection_observer::on_saved_tracks_status_received, ids);
            return true;
        }
        else
        {
            log::api->error("Failed to get saved status for {} tracks", ids.size());
        }
    }

    return false;
}
    
} // namespace spotify
} // namespace spotifar