#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
    using namespace utils;
    using namespace std::literals;
    using config::HotkeyID;

	Plugin::Plugin():
		api(),
        panel(api),
        player(api)
	{
        // TODO: what if not initialized?
		if (api.start())
        {
            panel.gotoRootMenu();
        }

        background_tasks.push_task([this] {
            on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
        });
        

        ObserverManager::subscribe<config::ConfigObserver>(this);
	}

	Plugin::~Plugin()
	{
        on_global_hotkeys_setting_changed(false);
        ObserverManager::unsubscribe<config::ConfigObserver>(this);
	}

    void Plugin::start()
    {
        launch_sync_worker();
    }

    void Plugin::shutdown()
    {
        shutdown_sync_worker();

        // when shutting down, Far closes ui itself
        player.hide(false);
        api.shutdown();
    }
    
    void Plugin::update_panel_info(OpenPanelInfo *info)
    {
        panel.update_panel_info(info);
    }
    
    intptr_t Plugin::update_panel_items(GetFindDataInfo *info)
    {
        return panel.update_panel_items(info);
    }
    
    void Plugin::free_panel_items(const FreeFindDataInfo *info)
    {
        panel.free_panel_items(info);
    }
    
    intptr_t Plugin::select_item(const SetDirectoryInfo *info)
    {
        return panel.select_item(info);
    }

    intptr_t Plugin::process_input(const ProcessPanelInputInfo *info)
    {
        auto &key_event = info->Rec.Event.KeyEvent;
        if (key_event.bKeyDown)
        {
            int key = utils::far3::input_record_to_combined_key(key_event);
            switch (key)
            {
                case VK_F3:
                {
                    return player.show();
                }
            }
        }
        return panel.process_input(info);
    }
    
    void Plugin::launch_sync_worker()
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

                    background_tasks.process_all();

                    check_global_hotkeys();

                    std::this_thread::sleep_for(50ms);
                }
            }
            catch (const std::exception &ex)
            {
                // TODO: what if there is an error, but no playback is opened
                exit_msg = ex.what();
                log::api->critical("An exception occured while syncing up with an API: {}", exit_msg);
            }
            
            // TODO: remove and cleanup the code
            // ObserverManager::notify(&BasicApiObserver::on_playback_sync_finished, exit_msg);
        });

        is_worker_listening = true;
        std::thread(std::move(task)).detach();
        log::api->info("An API sync worker has been launched");
    }

    void Plugin::shutdown_sync_worker()
    {
        is_worker_listening = false;
        
        // trying to acquare a sync worker mutex, giving worker time to clean up
        // all the resources
        const std::lock_guard worker_lock(sync_worker_mutex);
        log::api->info("An API sync worker has been stopped");
    }
    
    void Plugin::on_global_hotkeys_setting_changed(bool is_enabled)
    {
        background_tasks.push_task([is_enabled] {
            log::global->info("Changing global hotkeys state: {}", is_enabled);

            for (int idx = HotkeyID::PLAY; idx != HotkeyID::LAST; idx++)
            {
                HotkeyID hotkey_id = static_cast<HotkeyID>(idx);
                UnregisterHotKey(NULL, hotkey_id);

                if (is_enabled)
                {
                    auto *hotkey = config::get_hotkey(hotkey_id);
                    if (hotkey != nullptr && hotkey->first != far3::KEY_NONE)
                    {
                        if (RegisterHotKey(NULL, hotkey_id, hotkey->second | MOD_NOREPEAT, hotkey->first))
                        {
                            log::global->debug("A global hotkey is registered, {}, {}", hotkey->first, hotkey->second);
                        }
                        else
                        {
                            LPVOID lpMsgBuf;
                            FormatMessage(
                                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                GetLastError(),
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR) &lpMsgBuf,
                                0, NULL);
                            
                            if (NULL != lpMsgBuf)
                            {
                                log::global->error("There is an error while registering a hotkey: {}",
                                                   utils::to_string((LPCTSTR)lpMsgBuf));
                                LocalFree(lpMsgBuf);
                                lpMsgBuf = NULL;
                            }
                        }
                    }
                }
            }
        });
    }
    
    void Plugin::on_global_hotkey_changed(HotkeyID hotkey_id, WORD virtual_key, WORD modifiers)
    {
        // reinitialize all the hotkeys
        on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
    }
    
    void Plugin::check_global_hotkeys()
    {
        if (!config::is_global_hotkeys_enabled())
            return;

        MSG msg = {0};
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_HOTKEY)
        {
            switch (LOWORD(msg.wParam))
            {
                case HotkeyID::PLAY:
                    return api.toggle_playback();
                case HotkeyID::SKIP_NEXT:
                    return api.skip_to_next();
                case HotkeyID::SKIP_PREV:
                    return api.skip_to_previous();
            }
        }
    }
}