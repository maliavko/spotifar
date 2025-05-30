#include "playback.hpp"

namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;

bool playback_cache::is_active() const
{
    // the cache is actively synchronized only when the user is authenticated and there are
    // playback observers
    return api_proxy->is_authenticated();
}

clock_t::duration playback_cache::get_sync_interval() const
{
    // every second, minus some gap for smoother synching
    return utils::events::has_observers<playback_observer>() ? 950ms : 5s;
}

void playback_cache::on_data_synced(const playback_state_t &data, const playback_state_t &prev_data)
{
    if (data.item != prev_data.item)
        dispatch_event(&playback_observer::on_track_changed, data.item, prev_data.item);

    if (data.progress_ms != prev_data.progress_ms)
        dispatch_event(&playback_observer::on_track_progress_changed, data.item.duration, data.progress);

    if (data.device.volume_percent != prev_data.device.volume_percent)
        dispatch_event(&playback_observer::on_volume_changed, data.device.volume_percent);

    if (data.shuffle_state != prev_data.shuffle_state)
        dispatch_event(&playback_observer::on_shuffle_state_changed, data.shuffle_state);

    if (data.repeat_state != prev_data.repeat_state)
        dispatch_event(&playback_observer::on_repeat_state_changed, data.repeat_state);

    if (data.is_playing != prev_data.is_playing)
        dispatch_event(&playback_observer::on_state_changed, data.is_playing);

    if (data.context != prev_data.context)
        dispatch_event(&playback_observer::on_context_changed, data.context);

    if (data.actions != prev_data.actions)
        dispatch_event(&playback_observer::on_permissions_changed, data.actions);
}

bool playback_cache::request_data(playback_state_t &data)
{
    auto req = item_requester<playback_state_t>("/v1/me/player");
    if (req.execute(api_proxy->get_ptr()))
    {
        data = req.get();
        return true;
    }
    return false;
}

} // namespace spotify
} // namespace spotifar