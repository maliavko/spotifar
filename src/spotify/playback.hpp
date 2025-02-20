#ifndef PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#define PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#pragma once

#include "stdafx.h"
#include "cache.hpp"
#include "items.hpp"

namespace spotifar { namespace spotify {

class playback_cache: public json_cache<playback_state>
{
public:
    //playback_cache(api_abstract *api): json_cache(L"PlaybackState"), api_proxy(api) {}
    playback_cache(api_abstract *api): json_cache(L""), api_proxy(api) {}
    virtual ~playback_cache() { api_proxy = nullptr; }
    virtual bool is_active() const;

    /// @param tracks_uris list of spotify tracks' URIs
    void activate_super_shuffle(const std::vector<string> &tracks_uris);
protected:
    virtual void on_data_synced(const playback_state &data, const playback_state &prev_data);
    virtual bool request_data(playback_state &data);
    virtual clock_t::duration get_sync_interval() const;

private:
    api_abstract *api_proxy;
};

struct playback_observer: public BaseObserverProtocol
{
    /// @brief A track has changed
    /// @param track a new track, which jsut started playing
    virtual void on_track_changed(const track &track) {};

    /// @brief A track's progress has changed
    /// @param duration a total track duration in seconds
    /// @param progress a current playing position in seconds
    virtual void on_track_progress_changed(int duration, int progress) {};

    /// @brief A volume has changed
    virtual void on_volume_changed(int volume) {};

    /// @brief A shuffle state has changed
    virtual void on_shuffle_state_changed(bool shuffle_state) {};

    /// @brief A repeat state has changed
    virtual void on_repeat_state_changed(const string &repeat_state) {};

    /// @brief A playback state has changed: us playing or not
    virtual void on_state_changed(bool is_playing) {};

    /// @brief A playing context has changed
    virtual void on_context_changed(const context &ctx) {};

    /// @brief Playing permissions have changed
    virtual void on_permissions_changed(const spotify::actions &actions) {};
};

} // namespace spotify
} // namespace spotifar

#endif // PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7