#ifndef NOTIFICATIONS_HPP_14F21E1C_41D7_4085_906F_A15412652B82
#define NOTIFICATIONS_HPP_14F21E1C_41D7_4085_906F_A15412652B82
#pragma once

#include "stdafx.h"
#include "spotify/observer_protocols.hpp"

namespace spotifar { namespace ui {

namespace notifications
{
    /// @brief Shows a 'now-playing' pop-up with the current track info
    /// @param show_buttons to show extra functional buttons
    void show_now_playing(const spotify::track_t &track, bool show_buttons = false);

    /// @brief Shows pop-up with the list of recent releases, which built-in
    /// scan worker managed to find
    void show_releases_found(const spotify::recent_releases_t &releases);
}


struct notifications_observer: public BaseObserverProtocol
{
    virtual void show_now_playing(const spotify::track_t &track, bool show_buttons = false) {}

    virtual void show_releases_found(const spotify::recent_releases_t &releases) {}
};


/// @brief A class-helpers for showing app win toasts notifications in the
/// windows tray notifications area
class notifications_handler:
    public spotify::playback_observer,   // for showing up the windows notification when track has changed
    public spotify::releases_observer,
    public notifications_observer
{
public:
    notifications_handler(spotify::api_weak_ptr_t api): api_proxy(api) {}
    ~notifications_handler() { api_proxy.reset(); }

    bool start();
    bool shutdown();
protected:
    // notifications observer handlers
    void show_now_playing(const spotify::track_t &track, bool show_buttons = false) override;
    void show_releases_found(const spotify::recent_releases_t &releases) override;

    // playback handlers
    void on_track_changed(const spotify::track_t &track, const spotify::track_t &prev_track) override;

    // recent releases handlers
    void on_releases_sync_finished(const spotify::recent_releases_t releases) override;
private:
    spotify::api_weak_ptr_t api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // NOTIFICATIONS_HPP_14F21E1C_41D7_4085_906F_A15412652B82