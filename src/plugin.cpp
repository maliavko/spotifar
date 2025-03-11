#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "ui/events.hpp"

namespace spotifar {

using namespace utils;
namespace hotkeys = config::hotkeys;
namespace far3 = utils::far3;

plugin::plugin():
    api(),
    panel(&api),
    player(&api)
{
    ObserverManager::subscribe<config::config_observer>(this);
    ObserverManager::subscribe<spotify::auth_observer>(this);
    ObserverManager::subscribe<ui::ui_events_observer>(this);

    // TODO: what if not initialized?
    if (api.start())
        ui::events::show_root_view();

    background_tasks.push_task([this] {
        on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
    });
}

plugin::~plugin()
{
    on_global_hotkeys_setting_changed(false);

    ObserverManager::unsubscribe<spotify::auth_observer>(this);
    ObserverManager::unsubscribe<config::config_observer>(this);
    ObserverManager::unsubscribe<ui::ui_events_observer>(this);
}

void plugin::start()
{
    log::global->info("Spotifar plugin has started, version {}", far3::get_plugin_version());

    launch_sync_worker();
}

void plugin::shutdown()
{
    shutdown_sync_worker();

    player.hide();
    api.shutdown();

    librespot.shutdown();
}

void plugin::update_panel_info(OpenPanelInfo *info)
{
    panel.update_panel_info(info);
}

intptr_t plugin::update_panel_items(GetFindDataInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
        
    return panel.update_panel_items(info);
}

void plugin::free_panel_items(const FreeFindDataInfo *info)
{
    panel.free_panel_items(info);
}

intptr_t plugin::set_directory(const SetDirectoryInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
    
    return panel.select_directory(info);
}

intptr_t plugin::process_input(const ProcessPanelInputInfo *info)
{
    namespace keys = far3::keys;

    auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (far3::keys::make_combined(key_event))
        {
            case VK_F8:
            {
                api.clear_http_cache();
                log::global->debug("Cache has been cleared");
            }
            case keys::q + keys::mods::alt:
            {
                if (!player.is_visible())
                    player.show();
                return TRUE;
            }
            case VK_F12 + keys::mods::ctrl:
            {
                FarMenuItem items[] = {
                    { 0,                    L"Test1            Ctrl+F3" },
                    { MIF_CHECKED | L'▼',   L"Test2            Ctrl+F7" },
                    { 0,                    L"Test3            Cltr+F4" },
                    { 0,                    L"Test4            Ctrl+F5" },
                    { 0,                    L"Test5            Ctrl+F6" },
                };

                auto r = config::ps_info.Menu(
                    &FarMessageGuid,
                    {},
                    -1, -1, 0,
                    FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE,
                    L"Sort by",
                    {}, {}, {}, {},
                    items,
                    std::size(items)
                );
                spdlog::debug("menu result {}", r);

                return TRUE;
            }
        }
    }
    return panel.process_input(info);
}

intptr_t plugin::compare_items(const CompareInfo *info)
{
    return panel.compare_items(info);
}

void plugin::launch_sync_worker()
{
    std::packaged_task<void()> task([this]
    {
        string exit_msg = "";
        const std::lock_guard worker_lock(sync_worker_mutex);

        try
        {
            while (is_worker_listening)
            {
                api.tick();
                player.tick();
                librespot.tick();

                background_tasks.process_all(); // ticking background tasks if any

                check_global_hotkeys();

                std::this_thread::sleep_for(50ms);
            }
        }
        catch (const std::exception &ex)
        {
            // TODO: what if there is an error, but no playback is opened
            exit_msg = ex.what();
            log::api->critical("An exception occured in the background thread: {}", exit_msg);
        }
        
        // TODO: remove and cleanup the code
        // ObserverManager::notify(&BasicApiObserver::on_playback_sync_finished, exit_msg);
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
    const std::lock_guard worker_lock(sync_worker_mutex);
    log::api->info("Plugin's background thread has been stopped");
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
                if (hotkey != nullptr && hotkey->first != far3::keys::none)
                {
                    if (!RegisterHotKey(NULL, hotkey_id, hotkey->second | MOD_NOREPEAT, hotkey->first))
                    {
                        log::global->error("There is an error while registering a hotkey: {}",
                            utils::get_last_system_error());
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

void plugin::on_auth_status_changed(const spotify::auth &auth)
{
    if (auth.is_valid() && !librespot.is_launched())
        if (!librespot.launch(auth.access_token))
        {
            librespot.shutdown(); // cleaning up the allocated resources if any
            utils::far3::show_far_error_dlg(
                MFarMessageErrorStartup, L"There is a problem launching Librespot "
                "process, look into logs");
        }
}

void plugin::show_player_dialog()
{
    player.show();
}

void plugin::check_global_hotkeys()
{
    if (!config::is_global_hotkeys_enabled())
        return;

    // peeking the messages from the thread's queue for the hotkey's ones
    // and processing them
    MSG msg = {0};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_HOTKEY)
    {
        switch (LOWORD(msg.wParam))
        {
            case hotkeys::play: return api.toggle_playback();
            case hotkeys::skip_next: return api.skip_to_next();
            case hotkeys::skip_previous: return api.skip_to_previous();
            // TODO: finish up the commands
        }
    }
}

} // namespace spotifar