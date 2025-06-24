#include "playback_handler.hpp"
#include "config.hpp"
#include "lng.hpp"
#include "spotify/interfaces.hpp"
#include "ui/dialogs/menus.hpp"
#include "ui/events.hpp"

namespace spotifar {

using namespace utils;
using utils::far3::get_text;
using utils::far3::get_vtext;
using utils::far3::synchro_tasks::dispatch_event;
using utils::events::start_listening;
using utils::events::stop_listening;


playback_handler::playback_handler(spotify::api_weak_ptr_t api):  api_proxy(api)
{
    start_listening<spotify::devices_observer>(this);
    start_listening<spotify::auth_observer>(this);
    start_listening<config::config_observer>(this);
    start_listening<librespot_observer>(this);
}

playback_handler::~playback_handler()
{
    stop_listening<spotify::devices_observer>(this);
    stop_listening<spotify::auth_observer>(this);
    stop_listening<config::config_observer>(this);
    stop_listening<librespot_observer>(this);
}

void playback_handler::tick()
{
    librespot.tick();
}

void playback_handler::launch_librespot_process(const string &access_token)
{
    if (config::is_playback_backend_enabled())
    {
        wait_for_discovery = true;

        if (!librespot.start(access_token))
        {
            shutdown_librespot_process(); // cleaning up the allocated resources if any

            far3::show_far_error_dlg(MErrorLibrespotStartupUnexpected, L"", MShowLogs, []
            {
                far3::panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
            });
        }
    }

    // if playback backend is not started for any reason, trying to pick up any available device
    if (!librespot.is_running())
        pick_up_any();
}

void playback_handler::shutdown_librespot_process()
{
    wait_for_discovery = false;
    librespot.stop();
}

void playback_handler::on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal)
{
    if (auth && !is_renewal) // only if it is not token renewal
        librespot.start(auth.access_token);
}

void playback_handler::on_devices_changed(const spotify::devices_t &devices)
{
    // searching for an active device
    auto active_dev_it = std::find_if(
        devices.begin(), devices.end(), [](const auto &d) { return d.is_active; });

    // updating a currently active device pointer
    if (active_dev_it != devices.end())
        active_device = &*active_dev_it;

    if (wait_for_discovery)
    {
        const auto &device_name = librespot.get_device_name();

        // we're waiting for our `device_name` device
        // and trying to pick it up and transfer playback to
        for (const auto &device: devices)
        {
            if (device.name == device_name)
            {
                wait_for_discovery = false;

                // some other device is already active
                if (!device.is_active)
                {
                    // ...let's provide a choice for the user to pick it up or leave the active one untouched
                    const auto &message = get_vtext(MTransferPlaybackMessage, active_dev_it->name, device_name);
                    const wchar_t* msgs[] = {
                        get_text(MTransferPlaybackTitle), message.c_str()
                    };

                    if (config::ps_info.Message(
                        &MainGuid, &FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
                    ) != 0) // "would you like to transfer a playback?"
                        return;
                }

                log::librespot->info("A librespot process is found, trasferring playback...");
                
                // if (auto api = api_proxy.lock())
                //     api->transfer_playback(device.id, true);
                return;
            }
        }
    }
}

void playback_handler::on_playback_backend_setting_changed(bool is_enabled)
{
    if (is_enabled)
    {
        if (auto api = api_proxy.lock())
            if (const auto &auth_cache = api->get_auth_cache())
                launch_librespot_process(auth_cache->get_access_token());
    }
    else
    {
        shutdown_librespot_process();
    }
}

void playback_handler::on_playback_backend_configuration_changed()
{
    log::global->debug("on_playback_backend_configuration_changed");
    
    if (auto api = api_proxy.lock())
        if (const auto &auth_cache = api->get_auth_cache())
            librespot.restart(auth_cache->get_access_token());
}

void playback_handler::on_librespot_stopped(bool emergency)
{
    if (emergency)
    {
        far3::synchro_tasks::push([this] {
            far3::show_far_error_dlg(MErrorLibrespotStoppedUnexpectedly, L"", MRelaunch, [this]
            {
                if (auto api = api_proxy.lock())
                    librespot.start(api->get_auth_cache()->get_access_token());
            });
        }, "librespot-unexpected-stop, show error dialog task");
    }
}

bool playback_handler::pick_up_any()
{
    if (api_proxy.expired()) return false;

    auto api = api_proxy.lock();
    const auto &devices = api->get_available_devices(true);

    // searching for an active device if any
    auto active_dev_it = std::find_if(
        devices.begin(), devices.end(), [](const auto &d) { return d.is_active; });
    
    // if there is an active device already - no need to do anything
    if (active_dev_it != devices.end()) return false;

    // no available device found, warning the user
    if (devices.empty())
    {
        const wchar_t* msgs[] = {
            get_text(MTransferPlaybackTitle),
            get_text(MTransferNoAvailableDevices),
        };
        config::ps_info.Message(&MainGuid, &FarMessageGuid, FMSG_MB_OK, nullptr, msgs, std::size(msgs), 0);
        return false;
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
        &MainGuid, &FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
    ) == 0;

    if (should_transfer)
    {
        // offering user a choice to pick up a device from the list of available
        auto dev_idx = config::ps_info.Menu(
            &MainGuid, &FarMessageGuid, -1, -1, 0,
            FMENU_AUTOHIGHLIGHT, NULL, NULL, NULL, NULL, NULL,
            &items[0], items.size());
        
        if (auto api = api_proxy.lock(); api && dev_idx > -1)
        {
            const auto &dev = devices[dev_idx];
            log::librespot->info("Transferring playback to device `{}`", utils::to_string(dev.name));
            api->transfer_playback(dev.id, true);
            return true;
        }
    }
    return false;
}

} // namespace spotifar