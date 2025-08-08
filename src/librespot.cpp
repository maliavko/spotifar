#include "librespot.hpp"
#include "utils.hpp"
#include "config.hpp"

namespace spotifar {

using namespace utils;
using namespace spotify;
using utils::far3::synchro_tasks::dispatch_event;


#if defined (DEBUG)
    static const wstring device_name = L"librespot-debug";
#else
    static const wstring device_name = L"librespot";
#endif


bool librespot::start(const string &access_token)
{
    if (running) return true;

    SECURITY_ATTRIBUTES sa_attrs;
    ZeroMemory(&sa_attrs, sizeof(sa_attrs));

    sa_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa_attrs.bInheritHandle = TRUE;
    sa_attrs.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&pipe_read, &pipe_write, &sa_attrs, 0))
    {
        log::librespot->error("CreatePipe failed, {}", get_last_system_error());
        return false;
    }

    if (!SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0))
    {
        log::librespot->error("SetHandleInformation failed, {}", get_last_system_error());
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
    cmd << utils::format(L"{}\\librespot.exe", config::get_plugin_launch_folder());
    cmd << utils::format(L" --cache {}\\cache", config::get_plugin_data_folder());
    cmd << utils::format(L" --system-cache {}\\system-cache", config::get_plugin_data_folder());
    cmd << L" --name " << device_name;
    cmd << L" --device-type computer";
    cmd << L" --cache-size-limit 1.5G";
    cmd << L" --disable-discovery";
    cmd << L" --bitrate " << to_wstring(config::get_playback_bitrate());
    cmd << L" --volume-ctrl " << to_wstring(config::get_playback_volume_ctrl());
    
    auto pb_fmt = config::get_playback_format();
    cmd << L" --format " << to_wstring(pb_fmt);

    if (config::playback::format::does_support_dither(pb_fmt))
        cmd << L" --dither " << to_wstring(config::get_playback_dither());

    if (config::is_playback_normalisation_enabled())
        cmd << L" --enable-volume-normalisation";

    if (config::is_playback_autoplay_enabled())
        cmd << L" --autoplay on";

    //if (!config::is_gapless_playback_enabled())
    //    cmd << L" --disable-gapless";

    if (!config::is_playback_cache_enabled())
        cmd << L" --disable-audio-cache";

    cmd << L" --access-token " << to_wstring(access_token);

    /// logging Librespot's command line arguments for debugging purposes
    log::global->info("Starting Librespot process, {}", to_string(cmd.str()));

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
        log::librespot->error("CreateProcess failed, {}", get_last_system_error());
        return false;
    }

    DWORD dw_mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    SetNamedPipeHandleState(pipe_read, &dw_mode, NULL, NULL);

    running = true;

    subscribe();

    dispatch_event(&librespot_observer::on_librespot_started);

    return true;
}

void librespot::stop(bool emergency)
{
    if (!running) return;

    log::global->info("Stopping Librespot process");

    unsubscribe();

    running = false;
    device = device_t{};
    
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
    
    dispatch_event(&librespot_observer::on_librespot_stopped, emergency);
}

void librespot::tick()
{
    if (!running) return;
    
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

    // the process is shut down unexpectedly, cleaning up the resources and
    // preparing for a relaunch
    if (exit_code != STILL_ACTIVE && exit_code != STATUS_CONTROL_C_EXIT)
        stop(true);
}

const wstring& librespot::get_device_name()
{
    return device_name;
}

void librespot::subscribe()
{
    if (!wait_for_discovery)
    {
        wait_for_discovery = true;
        utils::events::start_listening<devices_observer>(this);
    }
}

void librespot::unsubscribe()
{
    if (wait_for_discovery)
    {
        wait_for_discovery = false;
        utils::events::stop_listening<devices_observer>(this);
    }
}

void librespot::on_devices_changed(const devices_t &devices)
{
    device_t active_dev;

    // we're waiting for our `device_name` device
    // and trying to pick it up and transfer playback to
    for (const auto &d: devices)
    {
        if (d.name == device_name)
            device = d;

        if (d.is_active)
            active_dev = d;
    }

    if (wait_for_discovery && device)
    {
        log::librespot->info("The Librespot device is discovered {}", device.id);

        unsubscribe();
        dispatch_event(&librespot_observer::on_librespot_discovered, device, active_dev);
    }
}

} // namespace spotifar