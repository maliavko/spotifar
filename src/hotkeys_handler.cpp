#include "hotkeys_handler.hpp"
#include "config.hpp"
#include "lng.hpp"
#include "librespot.hpp" // IWYU pragma: keep
#include "spotify/interfaces.hpp" // IWYU pragma: keep
#include "ui/notifications.hpp"

namespace spotifar {

namespace hotkeys = config::hotkeys;
using namespace utils;
using namespace spotify;
using utils::far3::get_text;

hotkeys_handler::hotkeys_handler(api_weak_ptr_t api): api_proxy(api)
{
    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
    
    events::start_listening<config::config_observer>(this);
}

hotkeys_handler::~hotkeys_handler()
{
    events::stop_listening<config::config_observer>(this);

    background_tasks.clear_tasks();
}

void hotkeys_handler::tick()
{
    // ticking background tasks if any
    background_tasks.process_all();

    // checking windows queue for some hotkeys click messages
    static MSG msg = {0};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_HOTKEY:
            {            
                if (!config::is_global_hotkeys_enabled()) return;

                switch (LOWORD(msg.wParam))
                {
                    case hotkeys::play:             return toggle_playback();
                    case hotkeys::skip_next:        return skip_to_next();
                    case hotkeys::skip_previous:    return skip_to_prev();
                    case hotkeys::seek_forward:     return seek_forward(15000); // ms
                    case hotkeys::seek_backward:    return seek_backward(15000);  // ms
                    case hotkeys::volume_up:        return volume_up(5);
                    case hotkeys::volume_down:      return volume_down(5);
                    case hotkeys::show_toast:
                    {
                        if (auto api = api_proxy.lock())
                        {
                            const auto &pstate = api->get_playback_state();
                            ui::notifications::show_now_playing(pstate.item, true);
                        }
                        return;
                    }
                }
                return;
            }
        }
    }
}

void hotkeys_handler::toggle_playback()
{
    if (auto api = api_proxy.lock())
    {
        item_id_t device_id = spotify::invalid_id;
        const auto &devices = api->get_devices_cache();

        if (auto device = devices->get_active_device())
        {
            device_id = device->id;
        }
        else if (auto device = devices->get_device_by_name(librespot::get_device_name()))
        {
            device_id = device->id;
        }

        if (device_id == invalid_id)
            return pick_up_any();

        if (const auto &pstate = api->get_playback_state(); !pstate.is_playing)
            return api->resume_playback(device_id);
        else
            return api->pause_playback(device_id);
    }
}

void hotkeys_handler::skip_to_next()
{
    if (auto api = api_proxy.lock(); api && get_active_device())
        api->skip_to_next();
}

void hotkeys_handler::skip_to_prev()
{
    if (auto api = api_proxy.lock(); api && get_active_device())
        return api->skip_to_previous();
}

void hotkeys_handler::volume_up(int step)
{
    if (auto api = api_proxy.lock(); api && get_active_device())
        if (const auto &pstate = api->get_playback_state())
            return api->set_playback_volume(std::min(pstate.device.volume_percent + step, 100));
}

void hotkeys_handler::volume_down(int step)
{
    if (auto api = api_proxy.lock(); api && get_active_device())
        if (const auto &pstate = api->get_playback_state())
            return api->set_playback_volume(std::max(pstate.device.volume_percent - 5, 0));
}

void hotkeys_handler::seek_forward(int step)
{
    if (auto api = api_proxy.lock(); api && get_active_device())
        if (const auto &pstate = api->get_playback_state())
            return api->seek_to_position(std::min(pstate.progress_ms + step, pstate.item.duration_ms));
}

void hotkeys_handler::seek_backward(int step)
{
    if (auto api = api_proxy.lock(); api && get_active_device())
        if (const auto &pstate = api->get_playback_state())
            return api->seek_to_position(std::max(pstate.progress_ms - step, 0));
}

const device_t* hotkeys_handler::get_active_device() const
{
    if (auto api = api_proxy.lock())
        if (auto devices = api->get_devices_cache())
            return devices->get_active_device();
    return nullptr;
}

void hotkeys_handler::pick_up_any()
{
    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();
    auto devices_cache = api->get_devices_cache(true);
    const auto &devices = devices_cache->get_all();
    
    // if there is an active device already - no need to do anything
    if (devices_cache->get_active_device()) return;

    if (devices.empty())
    {
        // no available device found, warn the user
        const wchar_t* msgs[] = {
            get_text(MTransferPlaybackTitle),
            get_text(MTransferNoAvailableDevices),
        };
        config::ps_info.Message(&guids::MainGuid, &guids::FarMessageGuid, FMSG_MB_OK, nullptr, msgs, std::size(msgs), 0);
        return;
    }

    std::vector<FarMenuItem> items;
    for (const auto &dev: devices)
        items.push_back({ MIF_NONE, dev.name.c_str() });

    const wchar_t* msgs[] = {
        get_text(MTransferPlaybackTitle),
        get_text(MTransferPlaybackInactiveMessage01),
        get_text(MTransferPlaybackInactiveMessage02),
    };

    // offering user to transfer a playback
    bool should_transfer = config::ps_info.Message(
        &guids::MainGuid, &guids::FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
    ) == 0;

    if (should_transfer)
    {
        // offering user a choice to pick up some device from the list of available
        auto dev_idx = config::ps_info.Menu(
            &guids::MainGuid, &guids::FarMessageGuid, -1, -1, 0,
            FMENU_AUTOHIGHLIGHT, NULL, NULL, NULL, NULL, NULL,
            &items[0], items.size());
        
        if (api && dev_idx > -1)
        {
            const auto &dev = devices[dev_idx];
            log::global->info("Transferring playback to device `{}`", utils::to_string(dev.name));
            devices_cache->transfer_playback(dev.id, true);
            return;
        }
    }
    return;
}

void hotkeys_handler::on_global_hotkeys_setting_changed(bool is_enabled)
{
    // the definition of the global hotkeys must be performed in the
    // the same thread, where the keys check is happening. So, here we push
    // enabling function to the background tasks queue
    background_tasks.push_task([is_enabled] {
        log::global->info("Changing global hotkeys state: {}", is_enabled);

        for (int hotkey_id = hotkeys::play; hotkey_id != hotkeys::last; hotkey_id++)
        {
            UnregisterHotKey(NULL, hotkey_id); // first, we unregister all the hotkeys

            if (is_enabled) // then reinitialize those, which are enabled
            {
                if (auto *hotkey = config::get_hotkey(hotkey_id); hotkey && hotkey->first != utils::keys::none)
                {
                    if (!RegisterHotKey(NULL, hotkey_id, hotkey->second | MOD_NOREPEAT, hotkey->first))
                    {
                        log::global->error("There is an error while registering a hotkey `{}`: {}",
                            utils::to_string(keys::vk_to_string(hotkey->first)), utils::get_last_system_error());
                        continue;
                    }
                    log::global->debug("A global hotkey is registered, {}, {}", hotkey->first, hotkey->second);
                }
            }
        }
    });
}

void hotkeys_handler::on_global_hotkey_changed(config::settings::hotkeys_t changed_keys)
{
    // reinitialize all hotkeys
    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
}

void hotkeys_handler::on_logging_verbocity_changed(bool is_verbose)
{
    log::enable_verbose_logs(is_verbose);
}

} // namespace spotifar