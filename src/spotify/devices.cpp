#include "stdafx.h"
#include "devices.hpp"
#include "observers.hpp"

namespace spotifar
{
    namespace spotify
    {
        bool DevicesCache::is_enabled() const
        {
            return api->is_authenticated() && api->get_playback_observers_count() > 0;
        }

        utils::ms DevicesCache::get_sync_interval() const
        {
            return utils::ms(1000);
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
            auto res = api->get_client().Get("/v1/me/player/devices");
            if (res->status == httplib::OK_200)
                json::parse(res->body).at("devices").get_to(data);

            return true;
        }
    }
}