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

#ifdef _DEBUG
    static const wstring device_name = L"librespot-debug";
#else
    static const wstring device_name = L"librespot";
#endif


bool playback_handler::start(const string &access_token)
{
    if (is_running) return true;
    
    ui::scoped_waiting waiting(MWaitingInitLibrespot);

    subscribe();

    SECURITY_ATTRIBUTES sa_attrs;
    ZeroMemory(&sa_attrs, sizeof(sa_attrs));

    sa_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa_attrs.bInheritHandle = TRUE;
    sa_attrs.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&pipe_read, &pipe_write, &sa_attrs, 0))
    {
        log::librespot->error("CreatePipe failed, {}", utils::get_last_system_error());
        return false;
    }

    if (!SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0))
    {
        log::librespot->error("SetHandleInformation failed, {}", utils::get_last_system_error());
        return false;
    }
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = pipe_write;
    si.hStdOutput = pipe_write;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    ZeroMemory(&pi, sizeof(pi));

    std::wstringstream cmd;

    // https://github.com/librespot-org/librespot/wiki/Options
    cmd << std::format(L"{}\\librespot.exe", config::get_plugin_launch_folder());
    cmd << std::format(L" --cache {}\\cache", config::get_plugin_data_folder());
    cmd << std::format(L" --system-cache {}\\system-cache", config::get_plugin_data_folder());
    cmd << L" --name " << device_name;
    cmd << L" --device-type computer";
    cmd << L" --cache-size-limit 1.5G";
    cmd << L" --disable-discovery";
    cmd << L" --bitrate " << utils::to_wstring(config::get_playback_bitrate());
    cmd << L" --volume-ctrl " << utils::to_wstring(config::get_playback_volume_ctrl());
    
    auto pb_fmt = config::get_playback_format();
    cmd << L" --format " << utils::to_wstring(pb_fmt);

    if (config::playback::format::does_support_dither(pb_fmt))
        cmd << L" --dither " << utils::to_wstring(config::get_playback_dither());

    if (config::is_playback_normalisation_enabled())
        cmd << L" --enable-volume-normalisation";

    if (config::is_playback_autoplay_enabled())
        cmd << L" --autoplay on";

    //if (!config::is_gapless_playback_enabled())
    //    cmd << L" --disable-gapless";

    if (!config::is_playback_cache_enabled())
        cmd << L" --disable-audio-cache";

    /// logging Librespot's command line arguments for debugging purposes
    log::global->info("Starting Librespot process, {}", utils::to_string(cmd.str()));

    cmd << L" --access-token " << utils::to_wstring(access_token);

    // while verbocity is extended, the process freezes and stutter heavily for some reason
    // if (config::is_verbose_logging_enabled())
    //     cmd << L" --verbose";

    if(!CreateProcess(
        NULL,
        &cmd.str()[0],
        NULL, // process handle not inheritable
        NULL, // thread handle not inheritable
        TRUE, // set handle inheritance to TRUE
        0,    // https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags
        NULL, // Use parent's environment block
        NULL, // Use parent's starting directory 
        &si,  // Pointer to STARTUPINFO structure
        &pi)  // Pointer to PROCESS_INFORMATION structure
    )
    {
        log::librespot->error("CreateProcess failed, {}", utils::get_last_system_error());
        return false;
    }

    DWORD dw_mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    SetNamedPipeHandleState(pipe_read, &dw_mode, NULL, NULL);

    is_running = true;

    dispatch_event(&playback_device_observer::on_running_state_changed, true);

    return true;
}

void playback_handler::shutdown()
{
    if (!is_running) return;

    is_running = false;
    
    ui::scoped_waiting waiting(MWaitingFiniLibrespot);

    unsubscribe();
    
    // sending a control stop event to the librespot process
    //GenerateConsoleCtrlEvent(CTRL_C_EVENT, pi.dwProcessId);
    
    // closing all the used handles
    if (pipe_read != NULL)
    {
        CloseHandle(pipe_read);
        pipe_read = NULL;
    }
    if (pipe_write != NULL)
    {
        CloseHandle(pipe_write);
        pipe_write = NULL;
    }
    if (pi.hProcess != NULL)
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        pi.hProcess = NULL;
    }
    if (pi.hThread != NULL)
    {
        CloseHandle(pi.hThread);
        pi.hThread = NULL;
    }

    dispatch_event(&playback_device_observer::on_running_state_changed, false);
}

void playback_handler::restart(const string &access_token)
{
    shutdown();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    start(access_token);
}

void playback_handler::tick()
{
    if (!is_running) return;
    
    // the algo below parses all the accumulated Librespot process messages and propagates
    // them into regular plugin's log file
    DWORD bytes_read;
    static CHAR buffer[512];
    BOOL success = FALSE;

    /// 1 - timestamp; 2 - message log level; 3 - the message itself
    static auto pattern = std::regex("\\[(.+) (\\w+) .+\\] (.+)");
    static auto pattern_not_implemented = std::regex("(not implemented:.*)");

    // stringstream is used to read output buffer line by line, it is static, as some lines
    // can be unfinished in the moment of parsing, so the ending should stay somewhere to be
    // concatanated later and parsed again
    static std::stringstream ss(std::ios_base::app | std::ios_base::in | std::ios_base::out);

    success = ReadFile(pipe_read, buffer, 512, &bytes_read, NULL);
    if (success && bytes_read != 0) // if the process's output buffer has something to read
    {
        ss.write(buffer, bytes_read);
    
        string sline;
        while (std::getline(ss, sline) && !ss.eof())
        {
            std::smatch match;
            if (std::regex_search(sline, match, pattern))
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

            // special tracer for particular `not implemented: ***` messages with errors
            if (std::regex_search(sline, match, pattern_not_implemented))
                log::librespot->warn(match[1].str());
        }

        if (!ss.good())
        {
            ss.clear();
            ss.str(sline);
        }
    }

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    if (exit_code != STILL_ACTIVE && exit_code != STATUS_CONTROL_C_EXIT)
    {
        // the process is shut down unexpectedly, cleaning up the resources and
        // preparing for a relaunch
        shutdown();

        far3::synchro_tasks::push([this] {
            far3::show_far_error_dlg(MErrorLibrespotStoppedUnexpectedly, L"", MRelaunch, [this]
            {
                if (auto api = api_proxy.lock(); auto auth = api->get_auth_cache())
                    start(auth->get_access_token());
            });
        }, "librespot-unexpected-stop, show error dialog task");
    }
}

void playback_handler::subscribe()
{
    if (!is_listening_devices)
    {
        is_listening_devices = true;
        utils::events::start_listening<devices_observer>(this);
    }
}

void playback_handler::unsubscribe()
{
    if (is_listening_devices)
    {
        is_listening_devices = false;
        utils::events::stop_listening<devices_observer>(this);
    }
}

void playback_handler::on_devices_changed(const spotify::devices_t &devices)
{
    if (api_proxy.expired() || !is_running) return;

    // searching for an active device if any
    auto active_dev_it = std::find_if(
        devices.begin(), devices.end(), [](const auto &d) { return d.is_active; });

    // we're waiting for our `device_name` device
    // and trying to pick it up and transfer playback to
    for (const auto &device: devices)
        if (device.name == device_name)
        {
            // stop listening after a correct device is detected
            unsubscribe();

            auto api = api_proxy.lock();

            // some other device is already active
            if (active_dev_it != devices.end() && active_dev_it->id != device.id)
            {
                // let's provide a choice for the user to pick it up or leave the active one untouched
                const auto &message = get_vtext(MTransferPlaybackMessage, active_dev_it->name, device_name);
                const wchar_t* msgs[] = {
                    get_text(MTransferPlaybackTitle), message.c_str()
                };

                bool need_to_transfer = config::ps_info.Message(
                    &MainGuid, &FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
                ) == 0;

                if (!need_to_transfer) return;
            }

            log::librespot->info("A librespot process is found, trasferring playback...");
            api->transfer_playback(device.id, true);
            return;
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
        
        if (auto api = api_proxy.lock(); dev_idx > -1)
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