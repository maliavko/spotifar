#include "history.hpp"

namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;

static const size_t max_history_size = 250;

play_history::play_history(api_interface *api):
    json_cache<history_items_t>(L"play_history"),
    api_proxy(api)
{
};

recently_played_tracks_ptr play_history::get_recently_played(std::int64_t after)
{
    return recently_played_tracks_ptr(new recently_played_tracks_t(
        api_proxy->get_ptr(), "/v1/me/player/recently-played", {
            { "after", std::to_string(after) }
        }));
}

bool play_history::is_active() const
{
    return api_proxy->is_authenticated();
}

clock_t::duration play_history::get_sync_interval() const
{
    return utils::events::has_observers<play_history_observer>() ? 5s : 2min;
}

void play_history::on_data_synced(const history_items_t &data, const history_items_t &prev_data)
{
    if (data.size() != prev_data.size())
        dispatch_event(&play_history_observer::on_items_changed);
}

bool play_history::request_data(history_items_t &data)
{
    data = get();

    auto last_sync_time = 0LL;
    if (data.size() > 0)
        last_sync_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            utils::get_timestamp(data[0].played_at)).count();

    // we request only the new items after the last request timestamp, then we take
    // the old items list and extend it from the front
    auto new_items = get_recently_played(last_sync_time);
    if (new_items->fetch() && new_items->size() > 0)
        data.insert(data.begin(), new_items->begin(), new_items->end());

    // truncating the history to the certain amount of entries
    if (data.size() > max_history_size)
    {
        data.resize(max_history_size);
        data.shrink_to_fit();
    }

    return true;
}

} // namespace spotify
} // namespace spotifar