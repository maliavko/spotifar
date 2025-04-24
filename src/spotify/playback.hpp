#ifndef PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#define PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#pragma once

#include "stdafx.h"
#include "common.hpp"
#include "items.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

class playback_cache: public json_cache<playback_state_t>
{
public:
    playback_cache(api_interface *api): json_cache(), api_proxy(api) {}
    ~playback_cache() { api_proxy = nullptr; }

    /// @param tracks_uris list of spotify tracks' URIs
    void activate_super_shuffle(const std::vector<string> &tracks_uris);
protected:
    bool is_active() const override;
    void on_data_synced(const playback_state_t &data, const playback_state_t &prev_data) override;
    bool request_data(playback_state_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;

private:
    api_interface *api_proxy;
};

struct playback_observer: public BaseObserverProtocol
{
    /// @brief A track has changed
    /// @param track a new track, which jsut started playing
    virtual void on_track_changed(const track_t &track) {}

    /// @brief A track's progress has changed
    /// @param duration a total track duration in seconds
    /// @param progress a current playing position in seconds
    virtual void on_track_progress_changed(int duration, int progress) {}

    /// @brief A volume has changed
    virtual void on_volume_changed(int volume) {}

    /// @brief A shuffle state has changed
    virtual void on_shuffle_state_changed(bool shuffle_state) {}

    /// @brief A repeat state has changed
    virtual void on_repeat_state_changed(const string &repeat_state) {}

    /// @brief A playback state has changed: us playing or not
    virtual void on_state_changed(bool is_playing) {}

    /// @brief A playing context has changed
    virtual void on_context_changed(const context_t &ctx) {}

    /// @brief Playing permissions have changed
    virtual void on_permissions_changed(const actions_t &actions) {}
};

} // namespace spotify
} // namespace spotifar

#endif // PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7