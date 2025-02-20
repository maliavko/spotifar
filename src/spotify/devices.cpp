#include "stdafx.h"
#include "devices.hpp"

namespace spotifar { namespace spotify {

namespace log = utils::log;
using utils::far3::synchro_tasks::dispatch_event;

bool devices_cache::is_active() const
{
    // the cache is actively synchronized only when the user is authenticated and there are
    // playback observers
    return api_proxy->is_authenticated() && api_proxy->is_frequent_syncs();
}

clock_t::duration devices_cache::get_sync_interval() const
{
    // every second, minus some gap for smoother synching
    return 950ms;
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
        json::parse(res->body).at("devices").get_to(data);

    return true;
}

void from_json(const json &j, device &d)
{
    j.at("id").get_to(d.id);
    j.at("is_active").get_to(d.is_active);
    j.at("type").get_to(d.type);
    j.at("supports_volume").get_to(d.supports_volume);

    d.volume_percent = j.value("volume_percent", 100);
    d.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const device &d)
{
    j = json{
        { "id", d.id },
        { "is_active", d.is_active },
        { "name", utils::utf8_encode(d.name) },
        { "type", d.type },
        { "volume_percent", d.volume_percent },
        { "supports_volume", d.supports_volume },
    };
}

string device::to_str() const
{
    return std::format("device(name={}, id={})", utils::to_string(name), id);
}

bool operator==(const device &lhs, const device &rhs)
{
    return lhs.id == rhs.id;
}

} // namespace spotify
} // namespace spotifar