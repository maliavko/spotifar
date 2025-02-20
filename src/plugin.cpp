#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "ui/events.hpp"
#include "ui/config_dialog.hpp"
#include "ui/views/artists.hpp"

namespace spotifar {

using namespace utils;
namespace hotkeys = config::hotkeys;

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
    {
        ui::events::show_root_view();
    }

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
    log::global->info("Spotifar plugin has started, version {}", utils::far3::get_plugin_version());

    launch_sync_worker();
}

void plugin::shutdown()
{
    shutdown_sync_worker();
    
    // sending a control stop event to the librespot process
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, pi.dwProcessId);
    
    CloseHandle(m_hChildStd_OUT_Rd);
    CloseHandle(m_hChildStd_OUT_Wr);

    player.hide();
    api.shutdown();
}

void plugin::update_panel_info(OpenPanelInfo *info)
{
    panel.update_panel_info(info);
}

intptr_t plugin::update_panel_items(GetFindDataInfo *info)
{
    return panel.update_panel_items(info);
}

void plugin::free_panel_items(const FreeFindDataInfo *info)
{
    panel.free_panel_items(info);
}

intptr_t plugin::select_item(const SetDirectoryInfo *info)
{
    return panel.select_item(info);
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
                api.tick(); // ticking spotify api
                player.tick(); // ticking player ui

                background_tasks.process_all(); // ticking background tasks if any

                check_global_hotkeys();
                check_librespot_messages();

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

intptr_t plugin::process_input(const ProcessPanelInputInfo *info)
{
    auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (utils::far3::input_record_to_combined_key(key_event))
        {
            case VK_F3:
            {
                show_player_dialog();
                return TRUE;
            }
            case VK_F6:
            {
                GenerateConsoleCtrlEvent(CTRL_C_EVENT, pi.dwProcessId);
                return TRUE;
            }
            case VK_F8:
            {
                api.clear_http_cache();
                log::global->debug("Cache has been cleared");
            }
        }
    }
    return panel.process_input(info);
}

void plugin::launch_librespot(const string &access_token)
{
    SECURITY_ATTRIBUTES saAttr;
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0))
    {
        // log error
        HRESULT_FROM_WIN32(GetLastError());
        return;
    }

    if (!SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        // log error
        HRESULT_FROM_WIN32(GetLastError());
        return;
    }
    
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdError = m_hChildStd_OUT_Wr;
    si.hStdOutput = m_hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    ZeroMemory(&pi, sizeof(pi));

    auto data_folder = config::get_plugin_data_folder();
    std::wstringstream cmd;
    cmd << std::format(L"{}\\librespot.exe", config::get_plugin_launch_folder());
    cmd << std::format(L" --cache {}\\cache", data_folder);
    cmd << std::format(L" --system-cache {}\\system-cache", data_folder);
    cmd << L" --access-token " << utils::to_wstring(access_token);
    cmd << L" --name librespot";
    //cmd << L" --disable-audio-cache";
    cmd << L" --bitrate 320";
    // cmd << L" --backend rodio";
    cmd << L" --disable-discovery";
    // cmd << L" --initial-volume 90";
    // cmd << L" --enable-volume-normalisation";
    
    if (config::is_verbose_logging_enabled())
        cmd << L" --verbose";

    // if( !CreateProcess(
    //     NULL,
    //     &cmd.str()[0],
    //     NULL,           // Process handle not inheritable
    //     NULL,           // Thread handle not inheritable
    //     TRUE,          // Set handle inheritance to FALSE
    //     0,              // https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags
    //     NULL,           // Use parent's environment block
    //     NULL,           // Use parent's starting directory 
    //     &si,            // Pointer to STARTUPINFO structure
    //     &pi )           // Pointer to PROCESS_INFORMATION structure
    // )
    // {
    //     printf( "CreateProcess failed (%d).\n", GetLastError() );
    //     return;
    // }


    DWORD dwMode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    SetNamedPipeHandleState(m_hChildStd_OUT_Rd, &dwMode, NULL, NULL);
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

            if (is_enabled)
            {
                auto *hotkey = config::get_hotkey(hotkey_id);
                if (hotkey != nullptr && hotkey->first != far3::keys::none)
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

void plugin::on_global_hotkey_changed(config::settings::hotkeys_t changed_keys)
{
    // reinitialize all the hotkeys
    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
}

void plugin::on_logging_verbocity_changed(bool is_verbose)
{
    log::enable_verbose_logs(is_verbose);
}

void plugin::on_auth_status_changed(const spotify::auth &auth)
{
    if (auth.is_valid())
    {
        launch_librespot(auth.access_token);
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

void plugin::check_librespot_messages()
{
    DWORD dwRead;
    CHAR chBuf[512];
    BOOL bSuccess = FALSE;

    static auto r = std::regex("\\[(.+) (\\w+) .+\\] (.+)");
    static std::stringstream ss(std::ios_base::app | std::ios_base::in | std::ios_base::out);

    bSuccess = ReadFile(m_hChildStd_OUT_Rd, chBuf, 512, &dwRead, NULL);
    if (bSuccess && dwRead != 0)
    {
        ss.write(chBuf, dwRead);
    
        string sline;
        while (std::getline(ss, sline) && !ss.eof())
        {
            std::smatch match;
            if (std::regex_search(sline, match, r))
            {
                if (match[2] == "INFO")
                    log::librespot->info(match[3].str());
                else if (match[2] == "TRACE" || match[2] == "DEBUG")
                    log::librespot->debug(match[3].str());
                else if (match[2] == "WARNING")
                    log::librespot->warn(match[3].str());
                else if (match[2] == "ERROR" || match[2] == "CRITICAL")
                    log::librespot->debug(match[3].str());
            }
        }

        if (!ss.good())
        {
            ss.clear();
            ss.str(sline);
        }
    }
}

} // namespace spotifar