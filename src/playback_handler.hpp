#ifndef PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#define PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#pragma once

#include "stdafx.h"
#include "librespot.hpp"
#include "config.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar {

class playback_handler:
    public spotify::auth_observer,
    public config::config_observer,
    public librespot_observer
{
public:
    playback_handler(spotify::api_weak_ptr_t api);
    ~playback_handler();

    void tick();

    void toggle_playback();
    void skip_to_next();
    void skip_to_prev();

    void pick_up_any();
protected:
    void launch_librespot_process(const string &access_token);
    void shutdown_librespot_process();

    auto get_active_device(const spotify::devices_t &) const -> const spotify::device_t*;

    // auth handlers
    void on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal) override;
    
    // config observer handlers
    void on_playback_backend_setting_changed(bool is_enabled) override;
    void on_playback_backend_configuration_changed() override;

    // librespot observer handlers
    void on_librespot_stopped(bool emergency) override;
    void on_librespot_discovered(const spotify::device_t &dev, const spotify::device_t &active_dev) override;
private:
    spotify::api_weak_ptr_t api_proxy;
    librespot librespot;
};

} // namespace spotifar

#endif // PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180