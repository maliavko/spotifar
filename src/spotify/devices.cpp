#include "devices.hpp"
#include "abstract/observers.hpp"
#include "ObserverManager.h"

namespace spotifar
{
    namespace spotify
    {
        std::chrono::seconds DevicesCache::get_sync_interval() const
        {
            return std::chrono::seconds(0);
        }
        
        void DevicesCache::on_data_synced(DevicesList &data)
        {
            ObserverManager::notify(&PlaybackObserver::on_devices_changed, data);
        }

        bool DevicesCache::request_data(DevicesList &data)
        {
            DevicesList devices;

            if (auto r = endpoint->Get("/v1/me/player/devices"))
                json::parse(r->body).at("devices").get_to(devices);
            
            bool has_devices_changed = !std::equal(
                devices.begin(), devices.end(), data.begin(), data.end(),
                [](const auto &a, const auto &b) { return a.id == b.id && a.is_active == b.is_active; });
            
            if (has_devices_changed)
            {
                data.assign(devices.begin(), devices.end());
                return true;
            }

            return false;
        }
    }
}