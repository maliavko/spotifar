#ifndef LIBRESPOT_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#define LIBRESPOT_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#pragma once

#include "stdafx.h"

namespace spotifar {

class librespot_handler
{
public:
    auto launch(const string &access_token) -> bool;
    auto shutdown() -> void;

    auto tick() -> void;
    auto is_launched() const -> bool { return is_running; }

private:
    bool is_running = false;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
};

} // namespace spotifar

#endif // LIBRESPOT_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180