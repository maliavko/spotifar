#ifndef PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#define PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#pragma once

#include "stdafx.h"
#include "librespot.hpp"
#include "config.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar {

class playback_handler:
    public spotify::devices_observer,
    public spotify::auth_observer,
    public config::config_observer
{
public:
    playback_handler(spotify::api_weak_ptr_t api);
    ~playback_handler();

    void tick();
    bool pick_up_any();
protected:
    bool launch_librespot_process(const string &access_token);
    void shutdown_librespot_process();

    // auth handlers
    void on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal) override;

    // devices handlers
    void on_devices_changed(const spotify::devices_t &devices) override;
    
    // config observer handlers
    void on_playback_backend_setting_changed(bool is_enabled) override;
    void on_playback_backend_configuration_changed() override;
private:
    spotify::api_weak_ptr_t api_proxy;
    librespot librespot;
};

struct playback_device_observer: public BaseObserverProtocol
{
    virtual void on_running_state_changed(bool is_running) {}
};

} // namespace spotifar

#endif // PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180