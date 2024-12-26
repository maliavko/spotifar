#include "devices.hpp"
#include "abstract/observers.hpp"
#include "ObserverManager.h"

namespace spotifar
{
    namespace spotify
    {
        std::chrono::milliseconds DevicesCache::get_sync_interval() const
        {
            return std::chrono::milliseconds(100);
        }
        
        void DevicesCache::on_data_synced(const DevicesList &data, const DevicesList &prev_data)
        {
            bool has_devices_changed = !std::equal(
                data.begin(), data.end(), prev_data.begin(), prev_data.end(),
                [](const auto &a, const auto &b) { return a.id == b.id && a.is_active == b.is_active; });
            
            if (has_devices_changed)
                ObserverManager::notify(&PlaybackObserver::on_devices_changed, data);
        }

        bool DevicesCache::request_data(DevicesList &data)
        {
            if (auto r = endpoint->Get("/v1/me/player/devices"))
                json::parse(r->body).at("devices").get_to(data);

            return true;
        }
    }
}