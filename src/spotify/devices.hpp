#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "abstract.hpp"
#include "items.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

class devices_cache: public json_cache<devices_t>
{
public:
    devices_cache(api_interface *api): json_cache(), api_proxy(api) {}
    ~devices_cache() { api_proxy = nullptr; }

    bool pick_up_device(const string &device_id = "");
protected:
    // `json_cache` class overloads
    bool is_active() const override;
    bool request_data(devices_t &data) override;
    void on_data_synced(const devices_t &data, const devices_t &prev_data) override;
    auto get_sync_interval() const -> clock_t::duration override;

private:
    api_interface *api_proxy;
};

struct devices_observer: public BaseObserverProtocol
{
    /// @brief A list of available devices has been changed
    virtual void on_devices_changed(const devices_t &devices) {}
};

} // namespace spotify
} // namespace spotifar

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC