#ifndef NOTIFICATIONS_HPP_14F21E1C_41D7_4085_906F_A15412652B82
#define NOTIFICATIONS_HPP_14F21E1C_41D7_4085_906F_A15412652B82
#pragma once

#include "stdafx.h"
#include "spotify/playback.hpp"

namespace spotifar { namespace ui {

/// @brief A class-helpers for showing app win toasts notifications in the
/// windows tray notifications area
class notifications:
    public spotify::playback_observer   // for showing up the windows notification when track has changed
{
public:
    notifications(spotify::api_proxy_ptr api): api_proxy(api) {}
    ~notifications() { api_proxy.reset(); }

    bool start();
    bool shutdown();
    
    void show_now_playing(const spotify::track_t &track, bool show_buttons = false);
protected:
    // playback handlers
    void on_track_changed(const spotify::track_t &track, const spotify::track_t &prev_track) override;
private:
    spotify::api_proxy_ptr api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // NOTIFICATIONS_HPP_14F21E1C_41D7_4085_906F_A15412652B82