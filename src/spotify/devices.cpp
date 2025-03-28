#include "stdafx.h"
#include "devices.hpp"

namespace spotifar { namespace spotify {

namespace log = utils::log;
using utils::far3::synchro_tasks::dispatch_event;

bool devices_cache::is_active() const
{
    // the cache is actively synchronized only when the user is authenticated and there are
    // playback observers
    return api_proxy->is_authenticated() && utils::events::has_observers<devices_observer>();
}

clock_t::duration devices_cache::get_sync_interval() const
{
    // every second, minus some gap for smoother synching
    return 950ms;
}

bool devices_cache::pick_up_device(const string &device_id)
{
    resync(true); // to make sure we have the latest data

    auto available_devices = get();
    auto device_it = std::find_if(available_devices.begin(), available_devices.end(),
        [&device_id](auto &d) { return d.is_active; });

    if (device_it != available_devices.end())
    {
        log::api->info("Some device is already active, {}", device_it->to_str());
        return true;
    }

    // let's transfer a playback to the device with the given id, if it is not selected already
    for (const auto &d: available_devices)
        if (d.id == device_id && !d.is_active)
        {
            log::api->debug("Picking up a device with a given {}", d.to_str());
            api_proxy->transfer_playback(device_id);
            return true;
        }

    if (available_devices.size() > 0)
    {
        auto device = available_devices[0];
        if (device.is_active)
            return true;
        
        log::api->debug("Picking up a first available device {}", device.to_str());
        api_proxy->transfer_playback(device.id);
        return true;
    }
    
    log::api->warn("No available devices for picking up, playback is not transferred");
    return false;
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
    auto res = api_proxy->get("/v1/me/player/devices");
    if (res->status == httplib::OK_200)
    {
        json2::Document document;
        json2::Value &body = document.Parse(res->body);
        
        from_json(body["devices"], data);
    }

    return true;
}

} // namespace spotify
} // namespace spotifar