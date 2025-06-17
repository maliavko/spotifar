#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "items.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

class devices_cache: public json_cache<devices_t>
{
public:
    devices_cache(api_interface *api): json_cache(), api_proxy(api) {}
    ~devices_cache() { api_proxy = nullptr; }
protected:
    // json_cache interface
    bool is_active() const override;
    bool request_data(devices_t &data) override;
    void on_data_synced(const devices_t &data, const devices_t &prev_data) override;
    auto get_sync_interval() const -> clock_t::duration override;

private:
    api_interface *api_proxy;
};

} // namespace spotify
} // namespace spotifar

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC