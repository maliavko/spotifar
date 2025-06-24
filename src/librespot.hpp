#ifndef LIBRESPOT_HPP_CA69540B_430C_4CB4_8151_B07626A78367
#define LIBRESPOT_HPP_CA69540B_430C_4CB4_8151_B07626A78367
#pragma once

#include "stdafx.h"

namespace spotifar {

class librespot
{
public:
    librespot();

    bool start(const string &access_token);
    void restart(const string &access_token);
    void stop(bool emergency = false);

    void tick();

    bool is_running() const { return running; }

    /// @brief Returns the name of the Librespot device 
    static const wstring& get_device_name();
private:
    bool running = false;
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
};

struct librespot_observer: public BaseObserverProtocol
{
    virtual void on_librespot_started() {}

    virtual void on_librespot_stopped(bool emergency) {}
};

} // namespace spotifar

#endif //LIBRESPOT_HPP_CA69540B_430C_4CB4_8151_B07626A78367