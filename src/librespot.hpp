#ifndef LIBRESPOT_HPP_CA69540B_430C_4CB4_8151_B07626A78367
#define LIBRESPOT_HPP_CA69540B_430C_4CB4_8151_B07626A78367
#pragma once

#include "stdafx.h"
#include "spotify/observer_protocols.hpp"

namespace spotifar {

class librespot:
    public spotify::devices_observer
{
public:
    bool start(const string &access_token);
    void restart(const string &access_token);
    void stop(bool emergency = false);

    void tick();

    bool is_running() const { return running; }
    bool is_discovered() const { return device.is_valid(); }
    auto get_device() const -> const spotify::device_t& { return device; }

    /// @brief Returns the name of the Librespot device 
    static const wstring& get_device_name();
protected:
    void subscribe();
    void unsubscribe();

    // devices handlers
    void on_devices_changed(const spotify::devices_t &) override;
private:
    bool running = false;
    bool wait_for_discovery = false;

    spotify::device_t device;
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
};

struct librespot_observer: public BaseObserverProtocol
{
    virtual void on_librespot_started() {}

    /// @param emergency singnals that the process was stopped unexpectedly
    virtual void on_librespot_stopped(bool emergency) {}

    /// @brief The process was discovered by Spotify relay and ready to be used
    /// @param dev librespot device object
    /// @param active_dev currently active device; can be invalid, can be the same as `dev`
    virtual void on_librespot_discovered(const spotify::device_t &dev, const spotify::device_t &active_dev) {}
};

} // namespace spotifar

#endif //LIBRESPOT_HPP_CA69540B_430C_4CB4_8151_B07626A78367