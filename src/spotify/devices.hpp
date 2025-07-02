#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "items.hpp"
#include "cache.hpp"
#include "interfaces.hpp"

namespace spotifar { namespace spotify {

class devices_cache:
    public json_cache<devices_t>,
    public devices_cache_interface
{
public:
    devices_cache(api_interface *api): json_cache(), api_proxy(api) {}
    ~devices_cache() { api_proxy = nullptr; }
    
    auto get_active_device() const -> const device_t* override;
    auto get_device_by_id(const item_id_t&) const -> const device_t* override;
    auto get_device_by_name(const wstring&) const -> const device_t* override;
    auto get_all() const -> const devices_t& override;

    void transfer_playback(const item_id_t &device_id, bool start_playing = false) override;
protected:
    // json_cache interface
    bool is_active() const override;
    bool request_data(devices_t &data) override;
    void on_data_synced(const devices_t &data, const devices_t &prev_data) override;
    auto get_sync_interval() const -> clock_t::duration override;

private:
    api_interface *api_proxy;
    std::atomic<bool> is_first_sync = true;
};

} // namespace spotify
} // namespace spotifar

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC