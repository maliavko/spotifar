#include "library.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;
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

bool library::is_track_saved(const item_id_t &track_id)
{
    const auto &tracks = get().tracks;

    if (const auto it = tracks.find(track_id); it != tracks.end())
        return it->second;
    
    {
        std::lock_guard<std::mutex> lock(data_access_guard);
        tracks_to_process.push_back(track_id);
    }

    return false;
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

        if (auto result = api_proxy->check_saved_tracks(ids); result.size() == ids.size())
        {
            library_statuses_t saved_status;
            for (size_t i = 0; i < ids.size(); ++i)
                saved_status.insert({ ids[i], result[i] });

            // extending the current data with the new saved statuses
            data = get();
            data.tracks.insert(saved_status.begin(), saved_status.end());

            // dispatching an event to notify observers about the new saved status
            dispatch_event(&collection_observer::on_saved_tracks_status_received, saved_status);
            return true;
        }
        else
        {
            log::api->error("Failed to get saved status for {} tracks", ids.size());
        }
    }

    return false;
}

void library::on_data_synced(const data_t &data, const data_t &prev_data)
{
    // ideally the event should've been fired from this method, however it would
    // require to recalculate a difference, which is known in the method above
}
    
} // namespace spotify
} // namespace spotifar