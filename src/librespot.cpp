#include "librespot.hpp"
#include "config.hpp"
#include "lng.hpp"

namespace spotifar {

using namespace utils;
using utils::far3::get_text;
using utils::far3::get_vtext;

static const wstring device_name = L"librespot";

bool librespot_handler::launch(const string &access_token)
{
    return false;
    if (is_running)
    {
        log::global->warn("The Librespot process is already running");
        return false;
    }

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

    if (!config::is_gapless_playback_enabled())
        cmd << L" --disable-gapless";

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
    
    utils::events::start_listening<devices_observer>(this);

    return true;
}

void librespot_handler::shutdown()
{
    if (!is_running)
    {
        log::global->warn("The Librespot process is already shutdown");
        return;
    }
    
    utils::events::stop_listening<devices_observer>(this);
    
    // sending a control stop event to the librespot process
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, pi.dwProcessId);
    
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
        CloseHandle(pi.hProcess);
        pi.hProcess = NULL;
    }
    if (pi.hThread != NULL)
    {
        CloseHandle(pi.hThread);
        pi.hThread = NULL;
    }

    is_running = false;
}

void librespot_handler::tick()
{
    if (!is_running) return;
    
    // the algo below parses all the accumulated Librespot process messages and propagates
    // them into regular plugin's log file
    DWORD dwRead;
    static CHAR chBuf[512];
    BOOL bSuccess = FALSE;

    /// 1 - do not remember; 2 - message log level; 3 - the message itself
    static auto pattern = std::regex("\\[(.+) (\\w+) .+\\] (.+)");

    // stringstream is used to read output buffer line by line, it is static, as some lines
    // can be unfinished in the moment of parsing, so the ending should stay somewhere to be
    // concatanated later and parsed again
    static std::stringstream ss(std::ios_base::app | std::ios_base::in | std::ios_base::out);

    bSuccess = ReadFile(pipe_read, chBuf, 512, &dwRead, NULL);
    if (bSuccess && dwRead != 0) // if the process's output buffer has something to read
    {
        ss.write(chBuf, dwRead);
    
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
        }

        if (!ss.good())
        {
            ss.clear();
            ss.str(sline);
        }
    }
}

void librespot_handler::on_devices_changed(const spotify::devices_t &devices)
{   
    // searching for an active device if any
    auto active_dev_it = std::find_if(
        devices.begin(), devices.end(), [](const auto &d) { return d.is_active; });

    for (const auto &device: devices)
        if (device.name == device_name)
        {
            // stop listening after a correct device is detected
            utils::events::stop_listening<devices_observer>(this);

            if (auto api = api_proxy.lock())
            {
                // some other device is already active
                if (active_dev_it != devices.end() && active_dev_it->id != device.id)
                {
                    // let's provide a choice for the user to pick it up or leave the active one untouched
                    auto message = get_vtext(MTransferPlaybackMessage, active_dev_it->name, device_name);
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
}

} // namespace spotifar