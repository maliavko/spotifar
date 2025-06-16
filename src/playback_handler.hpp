#ifndef PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#define PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#pragma once

#include "stdafx.h"
#include "spotify/interfaces.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar {

class playback_handler:
    public spotify::devices_observer
{
public:
    playback_handler(spotify::api_weak_ptr_t api):  api_proxy(api) {}

    bool start(const string &access_token);
    void restart(const string &access_token);
    void shutdown();

    void tick();
    bool is_started() const { return is_running; }
    bool pick_up_any();
protected:
    void subscribe();
    void unsubscribe();

    void on_devices_changed(const spotify::devices_t &devices) override;
private:
    spotify::api_weak_ptr_t api_proxy;
    std::atomic<bool> is_running = false;
    bool is_listening_devices = false;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
};

struct playback_device_observer: public BaseObserverProtocol
{
    virtual void on_running_state_changed(bool is_running) {}
};

} // namespace spotifar

#endif // PLAYBACK_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180