#include "librespot.hpp"
#include "config.hpp"

namespace spotifar {

using namespace utils;

bool librespot_handler::launch(const string &access_token)
{
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
    cmd << std::format(L"{}\\librespot.exe", config::get_plugin_launch_folder());
    cmd << std::format(L" --cache {}\\cache", config::get_plugin_data_folder());
    cmd << std::format(L" --system-cache {}\\system-cache", config::get_plugin_data_folder());
    cmd << L" --access-token " << utils::to_wstring(access_token);
    cmd << L" --name librespot";
    //cmd << L" --disable-audio-cache";
    cmd << L" --bitrate 320";
    // cmd << L" --backend rodio";
    cmd << L" --disable-discovery";
    // cmd << L" --initial-volume 90";
    cmd << L" --enable-volume-normalisation";
    
    if (config::is_verbose_logging_enabled())
        cmd << L" --verbose";

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

    return true;
}

void librespot_handler::shutdown()
{
    if (!is_running)
        return;
    
    // sending a control stop event to the librespot process
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, pi.dwProcessId);
    
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

    // Close process and thread handles
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
}

void librespot_handler::tick()
{
    if (!is_running)
        return;
    
    DWORD dwRead;
    static CHAR chBuf[512];
    BOOL bSuccess = FALSE;

    static auto r = std::regex("\\[(.+) (\\w+) .+\\] (.+)");
    static std::stringstream ss(std::ios_base::app | std::ios_base::in | std::ios_base::out);

    bSuccess = ReadFile(pipe_read, chBuf, 512, &dwRead, NULL);
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