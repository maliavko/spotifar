#include "plugin.h"
#include "config.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
    using namespace utils;
    using namespace std::literals;

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
	}

	Plugin::~Plugin()
	{
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
        auto& key_event = info->Rec.Event.KeyEvent;
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
            // clock::time_point now;
            string exit_msg = "";
            const std::lock_guard worker_lock(sync_worker_mutex);

            // MSG msg = {0};
            // RegisterHotKey(NULL, 333, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 0x53);
            // RegisterHotKey(NULL, 334, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 0x44);
            // auto s = RegisterHotKey(NULL, 334, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 0x44);
            
            // LPVOID lpMsgBuf;
            // DWORD dw = GetLastError();
            // FormatMessage(
            //     FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            //     FORMAT_MESSAGE_FROM_SYSTEM |
            //     FORMAT_MESSAGE_IGNORE_INSERTS,
            //     NULL,
            //     dw,
            //     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            //     (LPTSTR) &lpMsgBuf,
            //     0, NULL);
            // LPCTSTR str = (LPCTSTR)lpMsgBuf;

            try
            {
                while (is_worker_listening)
                {
                    api.tick();
                    player.tick();

                    std::this_thread::sleep_for(50ms);
                }
            }
            catch (const std::exception &ex)
            {
                // TODO: what if there is an error, but no playback is opened
                exit_msg = ex.what();
                log::api->critical("An exception occured while syncing up with an API: {}", exit_msg);
            }
            
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
    
    void Plugin::check_global_hotkeys()
    {
        // MSG msg = {0};
        // RegisterHotKey(NULL, 333, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 0x53);
        // RegisterHotKey(NULL, 334, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 0x44);

        // if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_HOTKEY)
        // {
        //     switch (LOWORD(msg.wParam))
        //     {
        //         case 333:
        //         case 334:
        //             log::api->debug("hotkey received {}", LOWORD(msg.wParam));
        //     }
        // }

        // UnregisterHotKey(NULL, 333);
        // UnregisterHotKey(NULL, 334);
    }
}