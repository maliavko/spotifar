#include "playback_handler.hpp"
#include "config.hpp"
#include "lng.hpp"
#include "spotify/interfaces.hpp"
#include "ui/dialogs/menus.hpp"
#include "ui/events.hpp"

namespace spotifar {

using namespace utils;
using namespace spotify;
using utils::far3::get_text;
using utils::far3::get_vtext;
using utils::far3::synchro_tasks::dispatch_event;
using utils::events::start_listening;
using utils::events::stop_listening;


playback_handler::playback_handler(api_weak_ptr_t api):  api_proxy(api)
{
    start_listening<auth_observer>(this);
    start_listening<config::config_observer>(this);
    start_listening<librespot_observer>(this);
}

playback_handler::~playback_handler()
{
    stop_listening<auth_observer>(this);
    stop_listening<config::config_observer>(this);
    stop_listening<librespot_observer>(this);

    shutdown_librespot_process();
}

void playback_handler::tick()
{
    librespot.tick();
}

void playback_handler::toggle_playback()
{
    if (auto api = api_proxy.lock())
    {
        const auto *active_device = get_active_device();

        item_id_t device_id = spotify::invalid_id;

        if (active_device)
        {
            device_id = active_device->id;
        }
        else if (librespot.is_running())
        {
            device_id = librespot.get_device().id;
        }

        if (device_id == invalid_id)
            return pick_up_any();

        if (const auto &pstate = api->get_playback_state(true); !pstate.is_playing)
            return api->resume_playback(device_id);
        else
            return api->pause_playback(device_id);
    }
}

void playback_handler::skip_to_next()
{
    if (auto api = api_proxy.lock())
        if (const auto *active_device = get_active_device())
            return api->skip_to_next();
}

void playback_handler::skip_to_prev()
{
    if (auto api = api_proxy.lock())
        if (const auto *active_device = get_active_device())
            return api->skip_to_previous();
}

void playback_handler::volume_up()
{
    if (auto api = api_proxy.lock())
        if (const auto *active_device = get_active_device())
            if (const auto &pstate = api->get_playback_state())
                return api->set_playback_volume(std::min(pstate.device.volume_percent + 5, 100));
}

void playback_handler::volume_down()
{
    if (auto api = api_proxy.lock())
        if (const auto *active_device = get_active_device())
            if (const auto &pstate = api->get_playback_state())
                return api->set_playback_volume(std::max(pstate.device.volume_percent - 5, 0));
}

void playback_handler::seek_forward()
{
    if (auto api = api_proxy.lock())
        if (const auto *active_device = get_active_device())
            if (const auto &pstate = api->get_playback_state())
                return api->seek_to_position(std::min(pstate.progress_ms + 15000, pstate.item.duration_ms));
}

void playback_handler::seek_backward()
{
    if (auto api = api_proxy.lock())
        if (const auto *active_device = get_active_device())
            if (const auto &pstate = api->get_playback_state())
                return api->seek_to_position(std::max(pstate.progress_ms - 15000, 0));
}

void playback_handler::launch_librespot_process(const string &access_token)
{
    ui::show_waiting(MWaitingInitLibrespot);

    if (config::is_playback_backend_enabled())
    {
        if (!librespot.start(access_token))
        {
            shutdown_librespot_process(); // cleaning up the allocated resources if any

            far3::show_far_error_dlg(MErrorLibrespotStartupUnexpected, L"", MShowLogs, []
            {
                far3::panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
            });
        }
    }

    // if playback backend is not started for any reason,
    // offering user to pick up any available one
    if (!librespot.is_running())
    {
        pick_up_any();
        ui::hide_waiting();
    }
    else
    {
        ui::show_waiting(MWaitingLibrespotDiscover);
    }
}

void playback_handler::shutdown_librespot_process()
{
    librespot.stop();
}

const device_t* playback_handler::get_active_device(bool is_forced) const
{
    if (auto api = api_proxy.lock())
    {
        const auto &devices = api->get_available_devices(is_forced);
        const auto &active_dev_it = std::find_if(
            devices.begin(), devices.end(), [](const auto &d) { return d.is_active; });

        if (active_dev_it != devices.end())
            return &*active_dev_it;
    }
    return nullptr;
}

void playback_handler::on_auth_status_changed(const auth_t &auth, bool is_renewal)
{
    if (auth && !is_renewal) // only if it is not token renewal
        launch_librespot_process(auth.access_token);
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
                    launch_librespot_process(api->get_auth_cache()->get_access_token());
            });
        }, "librespot-unexpected-stop, show error dialog task");
    }
}

void playback_handler::on_librespot_discovered(const device_t &dev, const device_t &active_dev)
{
    ui::hide_waiting();

    if (active_dev)
    {
        // we are done if the current active device is already Librespot
        if (active_dev.id == dev.id) return;

        // ... if it is some other device - we provide a choice for the user to pick it up or
        // leave the active one untouched
        const auto &message = get_vtext(MTransferPlaybackMessage, active_dev.name, dev.name);
        const wchar_t* msgs[] = {
            get_text(MTransferPlaybackTitle), message.c_str()
        };

        if (config::ps_info.Message(
            &MainGuid, &FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
        ) != 0) // "would you like to transfer a playback?" "Ok" button is "0"
            return;
    }

    log::librespot->info("A librespot process is found, trasferring playback...");
    
    if (auto api = api_proxy.lock())
        api->transfer_playback(dev.id, true);
}

void playback_handler::pick_up_any()
{
    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();
    
    const auto &devices = api->get_available_devices(true);
    const auto &active_device = get_active_device();
    
    // if there is an active device already - no need to do anything
    if (!active_device) return;

    // no available device found, warn the user
    if (devices.empty())
    {
        const wchar_t* msgs[] = {
            get_text(MTransferPlaybackTitle),
            get_text(MTransferNoAvailableDevices),
        };
        config::ps_info.Message(&MainGuid, &FarMessageGuid, FMSG_MB_OK, nullptr, msgs, std::size(msgs), 0);
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
        &MainGuid, &FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
    ) == 0;

    if (should_transfer)
    {
        // offering user a choice to pick up some device from the list of available
        auto dev_idx = config::ps_info.Menu(
            &MainGuid, &FarMessageGuid, -1, -1, 0,
            FMENU_AUTOHIGHLIGHT, NULL, NULL, NULL, NULL, NULL,
            &items[0], items.size());
        
        if (api && dev_idx > -1)
        {
            const auto &dev = devices[dev_idx];
            log::librespot->info("Transferring playback to device `{}`", utils::to_string(dev.name));
            api->transfer_playback(dev.id, true);
            return;
        }
    }
    return;
}

} // namespace spotifar