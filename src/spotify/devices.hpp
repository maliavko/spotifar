#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "cached_value.hpp"
#include "interfaces.hpp"
#include "items.hpp"

namespace spotifar
{
    namespace spotify
    {
        class DevicesCache: public CachedItem<DevicesList>
        {
        public:
            DevicesCache(IApi *api):
                CachedItem(L"DevicesList", false),
                api(api)
                {}

            virtual ~DevicesCache() { api = nullptr; }

        protected:
            virtual bool request_data(DevicesList &data);
            virtual void on_data_synced(const DevicesList &data, const DevicesList &prev_data);
            virtual utils::ms get_sync_interval() const;

        private:
            std::shared_ptr<spdlog::logger> logger;
            IApi *api;
        };
    }
}

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC