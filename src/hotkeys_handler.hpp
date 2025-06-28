#ifndef HOTKEYS_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#define HOTKEYS_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180
#pragma once

#include "stdafx.h"
#include "config.hpp"

namespace spotifar {

class hotkeys_handler:
    public config::config_observer
{
public:
    hotkeys_handler(spotify::api_weak_ptr_t api);
    ~hotkeys_handler();
    
    void toggle_playback();
    void skip_to_next();
    void skip_to_prev();
    void volume_up(int step);
    void volume_down(int step);
    void seek_forward(int step);
    void seek_backward(int step);

    void tick();
protected:
    /// @brief Returns a currently active device or nullptr
    auto get_active_device() const -> const spotify::device_t*;

    /// @brief In case there is no active device and Librespot is not active
    /// for any reason, this method provides a fallback solution to offer
    /// for user to pick up manually a device to tranfer playback to from the list
    /// of available
    void pick_up_any();
    
    // config handlers
    void on_global_hotkeys_setting_changed(bool is_enabled) override;
    void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys) override;
    void on_logging_verbocity_changed(bool is_verbose) override;
private:
    spotify::api_weak_ptr_t api_proxy;
    
    // hotkeys registration and checks should happen in the same thread,
    // we create a separate tasks queue, to perform its evaluations
    utils::tasks_queue background_tasks;
};

} // namespace spotifar

#endif // HOTKEYS_HANDLER_HPP_5549F561_6D96_4E39_B9AB_40BA0947A180