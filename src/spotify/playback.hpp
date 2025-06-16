#ifndef PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#define PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#pragma once

#include "cache.hpp"
#include "interfaces.hpp"

namespace spotifar { namespace spotify {

class playback_cache: public json_cache<playback_state_t>
{
public:
    playback_cache(api_interface *api): json_cache(), api_proxy(api) {}
    ~playback_cache() { api_proxy = nullptr; }
protected:
    bool is_active() const override;
    void on_data_synced(const playback_state_t &data, const playback_state_t &prev_data) override;
    bool request_data(playback_state_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;

private:
    api_interface *api_proxy;
};

} // namespace spotify
} // namespace spotifar

#endif // PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7