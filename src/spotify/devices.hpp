#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "cache.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

class devices_cache: public json_cache<devices_t>
{
public:
    // devices_cache(api_abstract *api): json_cache(L"DevicesList"), api_proxy(api) {}
    devices_cache(api_abstract *api): json_cache(L""), api_proxy(api) {}
    virtual ~devices_cache() { api_proxy = nullptr; }
    virtual bool is_active() const;
protected:
    virtual bool request_data(devices_t &data);
    virtual void on_data_synced(const devices_t &data, const devices_t &prev_data);
    virtual clock_t::duration get_sync_interval() const;

private:
    api_abstract *api_proxy;
};

struct devices_observer: public BaseObserverProtocol
{
    /// @brief A list of available devices has been changed
    virtual void on_devices_changed(const devices_t &devices) {};
};

} // namespace spotify
} // namespace spotifar

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC