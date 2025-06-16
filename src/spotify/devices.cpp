#include "stdafx.h"
#include "devices.hpp"
#include "requesters.hpp"
#include "observer_protocols.hpp"

namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;

bool devices_cache::is_active() const
{
    // the cache is actively synchronized only when the user is authenticated and there are
    // playback observers
    return api_proxy->is_authenticated() && utils::events::has_observers<devices_observer>();
}

clock_t::duration devices_cache::get_sync_interval() const
{
    return 3s;
}

void devices_cache::on_data_synced(const devices_t &data, const devices_t &prev_data)
{
    bool has_devices_changed = !std::equal(
        data.begin(), data.end(), prev_data.begin(), prev_data.end(),
        [](const auto &a, const auto &b) { return a.id == b.id && a.is_active == b.is_active; });
    
    if (has_devices_changed)
        dispatch_event(&devices_observer::on_devices_changed, data);
}

bool devices_cache::request_data(devices_t &data)
{
    auto r = item_requester<devices_t>("/v1/me/player/devices", {}, "devices");
    if (r.execute(api_proxy->get_ptr()))
    {
        data = r.get();
        return true;
    }
    return false;
}

} // namespace spotify
} // namespace spotifar