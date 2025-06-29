#include "stdafx.h"
#include "devices.hpp"
#include "requesters.hpp"
#include "observer_protocols.hpp"

namespace spotifar { namespace spotify {

using namespace utils;
using utils::events::has_observers;
using utils::far3::synchro_tasks::dispatch_event;

static const device_t* find_device(const devices_t &devices, std::function<bool(const device_t&)> predicate)
{
    const auto it = std::find_if(devices.begin(), devices.end(), predicate);
    return it != devices.end() ? &*it : nullptr;
}

const device_t* devices_cache::get_active_device() const
{
    return find_device(get(), [](const auto &d) { return d.is_active; });
}

const device_t* devices_cache::get_device_by_id(const item_id_t &dev_id) const
{
    return find_device(get(), [dev_id](const auto &d) { return d.id == dev_id; });
}

const device_t* devices_cache::get_device_by_name(const wstring &name) const
{
    return find_device(get(), [name](const auto &d) { return d.name == name; });
}

const devices_t& devices_cache::get_all() const
{
    return get();
}

void devices_cache::transfer_playback(const item_id_t &device_id, bool start_playing)
{
    const auto &devices = get();
    const auto device_it = std::find_if(devices.begin(), devices.end(),
        [&device_id](const auto &d) { return d.id == device_id; });

    if (device_it == devices.end())
        return playback_cmd_error("There is no devices with the given id={}", device_id);

    if (device_it->is_active)
        return playback_cmd_error("The given device is already active, {}", device_it->to_str());
    
    api_proxy->get_pool().detach_task(
        [
            this, start_playing, dev_id = std::as_const(device_id),
            dev_idx = std::distance(devices.begin(), device_it)
        ]
        {
            http::json_body_builder body;

            body.object([&]
            {
                body.insert("device_ids", { dev_id });
                body.insert("play", start_playing);
            });

            auto requester = put_requester("/v1/me/player", body.str());
            if (requester.execute(api_proxy->get_ptr()))
            {
                this->patch([dev_idx](auto &d) {
                    json::Pointer(std::format("/{}/is_active", dev_idx)).Set(d, true);
                });
                this->resync(true);
            }
            else
                playback_cmd_error(http::get_status_message(requester.get_response()));
        });
}

bool devices_cache::is_active() const
{
    // the cache is actively synchronized only when the user is authenticated and there are
    // playback observers
    if (auto auth = api_proxy->get_auth_cache())
        return auth->is_authenticated() && has_observers<devices_observer>();
    return false;
}

clock_t::duration devices_cache::get_sync_interval() const
{
    return 3s;
}

void devices_cache::on_data_synced(const devices_t &data, const devices_t &prev_data)
{
    bool has_devices_changed = true;

    if (data.size() == prev_data.size())
        has_devices_changed = !std::equal(
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