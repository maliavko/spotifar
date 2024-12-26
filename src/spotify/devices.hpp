#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "abstract/cached_value.hpp"
#include "items.hpp"

namespace spotifar
{
    namespace spotify
    {
        class DevicesCache: public CachedValue<DevicesList>
        {
        public:
            DevicesCache(httplib::Client *endpoint):
                CachedValue(endpoint, L"DevicesList", false)
                {}

        protected:
            virtual bool request_data(DevicesList &data);
            virtual void on_data_synced(const DevicesList &data, const DevicesList &prev_data);
            virtual std::chrono::milliseconds get_sync_interval() const;

        private:
            std::shared_ptr<spdlog::logger> logger;
        };
    }
}

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC