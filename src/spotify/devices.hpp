#ifndef DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#define DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC
#pragma once

#include "cache.hpp"

namespace spotifar { namespace spotify {

struct device
{
    string id = "";
    bool is_active = false;
    wstring name;
    string type;
    int volume_percent = 100;
    bool supports_volume = false;

    string to_str() const;
    friend bool operator==(const device &lhs, const device &rhs);
    friend void from_json(const json &j, device &d);
    friend void to_json(json &j, const device &d);
};

typedef std::vector<device> devices_list_t;

class devices_cache: public json_cache<devices_list_t>
{
public:
    devices_cache(api_abstract *api): json_cache(L"DevicesList"), api(api) {}
    virtual ~devices_cache() { api = nullptr; }
    virtual bool is_active() const;
protected:
    virtual bool request_data(devices_list_t &data);
    virtual void on_data_synced(const devices_list_t &data, const devices_list_t &prev_data);
    virtual clock_t::duration get_sync_interval() const;

private:
    api_abstract *api;
};

struct devices_observer: public BaseObserverProtocol
{
    /// @brief A list of available devices has been changed
    virtual void on_devices_changed(const devices_list_t &devices) {};
};

} // namespace spotify
} // namespace spotifar

#endif // DEVICES_HPP_C39A5DF6_0432_4CF0_ADA2_10DA51FB40DC