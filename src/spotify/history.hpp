#ifndef CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#define CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#pragma once

#include "abstract.hpp"
#include "items.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

class play_history: public json_cache<history_items_t>
{
public:
    play_history(api_abstract *api);
    ~play_history() { api_proxy = nullptr; }
protected:
    bool is_active() const;
    bool request_data(history_items_t &data);
    auto get_sync_interval() const -> clock_t::duration;
    void on_data_synced(const history_items_t &data, const history_items_t &prev_data);
private:
    api_abstract *api_proxy;
};

struct play_history_observer: public BaseObserverProtocol
{
    virtual void on_items_changed() {};
};

} // namespace spotify
} // namespace spotifar

#endif //CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72