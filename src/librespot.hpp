#ifndef LIBRESPOT_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#define LIBRESPOT_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#pragma once

#include "stdafx.h"
#include "spotify/devices.hpp"

namespace spotifar {

class librespot_handler:
    public spotify::devices_observer // to wait for the Librespot device get available and pick it up
{
public:
    librespot_handler(spotify::api_proxy_ptr api): api_proxy(api) {}
    ~librespot_handler() { api_proxy.reset(); }

    auto start(const string &access_token) -> bool;
    void shutdown();

    void tick();
    auto is_started() const -> bool { return is_running; }
protected:
    void on_devices_changed(const spotify::devices_t &devices) override;
private:
    spotify::api_proxy_ptr api_proxy;
    bool is_running = false;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
};

} // namespace spotifar

#endif // LIBRESPOT_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180