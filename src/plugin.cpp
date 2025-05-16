#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar {

using namespace utils;
namespace hotkeys = config::hotkeys;
namespace far3 = utils::far3;


plugin::plugin(): api(new spotify::api())
{
    panel = std::make_unique<ui::panel>(api);
    player = std::make_unique<ui::player>(api);
    notifications = std::make_unique<ui::notifications>(api);
    librespot = std::make_unique<librespot_handler>(api);

    utils::events::start_listening<config::config_observer>(this);
    utils::events::start_listening<spotify::auth_observer>(this);
    utils::events::start_listening<spotify::releases_observer>(this);
    utils::events::start_listening<ui::ui_events_observer>(this);
}

plugin::~plugin()
{
    utils::events::stop_listening<spotify::releases_observer>(this);
    utils::events::stop_listening<spotify::auth_observer>(this);
    utils::events::stop_listening<config::config_observer>(this);
    utils::events::stop_listening<ui::ui_events_observer>(this);

    panel.reset();
    player.reset();
    api.reset();
    librespot.reset();
    notifications.reset();
}

void plugin::start()
{
    log::global->info("Spotifar plugin has started, version {}", far3::get_plugin_version());
    
    api->start();
    notifications->start();

    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
    
    launch_sync_worker();
}

void plugin::shutdown()
{
    try
    {
        player->hide();

        background_tasks.clear_tasks();

        api->shutdown();
        notifications->shutdown();

        shutdown_sync_worker();
        shutdown_librespot_process();

    }
    catch (const std::exception &ex)
    {
        log::global->warn("There is an error, while shutting down the "
            "plugin: {}", ex.what());
    }
}

void plugin::update_panel_info(OpenPanelInfo *info)
{
    panel->update_panel_info(info);
}

intptr_t plugin::update_panel_items(GetFindDataInfo *info)
{
    // plugin does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
        
    return panel->update_panel_items(info);
}

void plugin::free_panel_items(const FreeFindDataInfo *info)
{
    panel->free_panel_items(info);
}

intptr_t plugin::set_directory(const SetDirectoryInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
    
    return panel->select_directory(info);
}

intptr_t plugin::process_input(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (keys::make_combined(key_event))
        {
            case keys::q + keys::mods::alt:
            {
                if (!player->is_visible())
                    player->show();
                return TRUE;
            }
        }
    }
    return panel->process_input(info);
}

intptr_t plugin::compare_items(const CompareInfo *info)
{
    return panel->compare_items(info);
}

void plugin::launch_sync_worker()
{
    std::packaged_task<void()> task([this]
    {
        try
        {
            std::lock_guard worker_lock(sync_worker_mutex);
            clock_t::time_point last_tick = clock_t::now();

            while (is_worker_listening)
            {
                const auto &now = clock_t::now();
                const auto &delta = now - last_tick;

                api->tick();
                player->tick();
                librespot->tick();

                background_tasks.process_all(); // ticking background tasks if any

                process_win_messages_queue();

                log::tick(delta);

                std::this_thread::sleep_for(50ms);
                last_tick = now;
            }
        }
        catch (const std::exception &ex)
        {
            far3::synchro_tasks::push([ex] {
                far3::show_far_error_dlg(MErrorSyncThreadFailed, to_wstring(ex.what()));
                far3::panels::quit(PANEL_ACTIVE);
            });
        }
    });

    is_worker_listening = true;
    std::thread(std::move(task)).detach();
    log::api->info("Plugin's background thread has been launched");
}

void plugin::shutdown_sync_worker()
{
    is_worker_listening = false;
    
    // trying to acquare a sync worker mutex, giving worker time to clean up
    // all the resources
    std::lock_guard worker_lock(sync_worker_mutex);
    log::api->info("Plugin's background thread has been stopped");
}

void plugin::launch_librespot_process(const string &access_token)
{
    if (!config::is_playback_backend_enabled()) return;

    if (librespot != nullptr && !librespot->is_started() && !librespot->start(access_token))
    {
        shutdown_librespot_process(); // cleaning up the allocated resources if any
        far3::show_far_error_dlg(MErrorLibrespotStartupUnexpected, L"", MShowLogs, []
        {
            far3::panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
        });
    }
}
void plugin::shutdown_librespot_process()
{
    if (librespot != nullptr && librespot->is_started())
        librespot->shutdown();
}

void plugin::on_global_hotkeys_setting_changed(bool is_enabled)
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
                auto *hotkey = config::get_hotkey(hotkey_id);
                if (hotkey != nullptr && hotkey->first != utils::keys::none)
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

void plugin::on_global_hotkey_changed(config::settings::hotkeys_t changed_keys)
{
    // reinitialize all hotkeys
    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
}

void plugin::on_logging_verbocity_changed(bool is_verbose)
{
    log::enable_verbose_logs(is_verbose);
}

void plugin::on_playback_backend_setting_changed(bool is_enabled)
{
    log::global->debug("on_playback_backend_setting_changed, {}", is_enabled);

    if (is_enabled)
    {
        const auto &auth = api->get_auth_data();
        if (auth.is_valid())
            launch_librespot_process(auth.access_token);
    }
    else
        shutdown_librespot_process();
}

void plugin::on_playback_backend_configuration_changed()
{
    log::global->debug("on_playback_backend_configuration_changed");
    
    const auto &auth = api->get_auth_data();
    if (librespot != nullptr && auth.is_valid())
    {
        // restart external process routine: shutdown, wait for the better and start over
        shutdown_librespot_process();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        launch_librespot_process(auth.access_token);
    }
}

void plugin::on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal)
{
    if (auth.is_valid() && !is_renewal) // only if it is not token renewal
    {
        launch_librespot_process(auth.access_token);
        
        // after first valid authentication we show root view
        ui::events::show_root(api);
        ui::events::refresh_panels();
    }
}

void plugin::on_releases_sync_finished(const spotify::recent_releases_t releases)
{
    notifications->show_recent_releases_found(releases);
}

void plugin::show_player()
{
    player->show();
}

void plugin::process_win_messages_queue()
{
    MSG msg = {0};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_HOTKEY:
            {            
                if (!config::is_global_hotkeys_enabled())
                    return;

                switch (LOWORD(msg.wParam))
                {
                    case hotkeys::play: return api->toggle_playback();
                    case hotkeys::skip_next: return api->skip_to_next();
                    case hotkeys::skip_previous: return api->skip_to_previous();
                    case hotkeys::seek_forward: return player->on_seek_forward_btn_clicked();
                    case hotkeys::seek_backward: return player->on_seek_backward_btn_clicked();
                    case hotkeys::volume_up: return player->on_volume_up_btn_clicked();
                    case hotkeys::volume_down: return player->on_volume_down_btn_clicked();
                    case hotkeys::show_toast:
                    {
                        const auto &pstate = api->get_playback_state(true);
                        if (!pstate.is_empty() && notifications != nullptr)
                            return notifications->show_now_playing(pstate.item, true);
                        return;
                    }
                }
                return;
            }
        }
    }
}

} // namespace spotifar