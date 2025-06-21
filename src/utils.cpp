#include "utils.hpp"
#include "config.hpp"
#include "lng.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"

bool operator==(const FarKey &lhs, const FarKey &rhs)
{
    return (lhs.VirtualKeyCode == rhs.VirtualKeyCode &&
        lhs.ControlKeyState == rhs.ControlKeyState);
}

namespace spotifar { namespace utils {

string format_number(uintmax_t num, float divider, const char units[8], float precision)
{
    if (num == 0) return "0" + units[0];

    std::ostringstream os;

    int o{};
    double mantissa = (double)num;
    for (; mantissa >= divider; mantissa /= divider, ++o);
    os << std::ceil(mantissa * precision) / precision << units[o];
    return os.str();
}

wstring trunc(const wstring &str, size_t size_to_cut)
{
    if (str.size() > size_to_cut)
        return str.substr(0, size_to_cut-1) + L'…';
    return str;
}

HINSTANCE open_web_browser(const string &address)
{ 
    log::global->debug("Redirecting to the external browser, {}", address);
    return ShellExecuteA(NULL, "open", address.c_str(), 0, 0, SW_SHOW);
}

namespace keys
{
    bool is_pressed(int virtual_key)
    {
        return GetKeyState(virtual_key) < 0;
    }

    static int make_combined(WORD virtual_key_code, DWORD control_key_state)
    {
        int key = static_cast<int>(virtual_key_code);
        const auto &state = control_key_state;
        
        if (state & RIGHT_CTRL_PRESSED || state & LEFT_CTRL_PRESSED) key |= keys::mods::ctrl;
        if (state & RIGHT_ALT_PRESSED || state & LEFT_ALT_PRESSED) key |= keys::mods::alt;
        if (state & SHIFT_PRESSED) key |= keys::mods::shift;

        return key;
    }

    int make_combined(const KEY_EVENT_RECORD &kir)
    {
        return make_combined(kir.wVirtualKeyCode, kir.dwControlKeyState);
    }
    
    int make_combined(const FarKey &fkey)
    {
        return make_combined(fkey.VirtualKeyCode, fkey.ControlKeyState);
    }
    
    wstring combined_to_string(int combined_key)
    {
        std::vector<wstring> keys;

        if (combined_key & mods::ctrl)
            keys.push_back(L"Ctrl");

        if (combined_key & mods::alt)
            keys.push_back(L"Alt");

        if (combined_key & mods::shift)
            keys.push_back(L"Shift");

        keys.push_back(vk_to_string(LOWORD(combined_key)));
            
        return string_join(keys, L"+");
    }

    wstring vk_to_string(WORD virtual_key_code)
    {
        // if the key is a text key
        if (virtual_key_code >= keys::a && virtual_key_code <= keys::z)
            return wstring(1, (char)virtual_key_code);

        // if it is a special key
        switch (virtual_key_code)
        {
            case VK_DOWN: return L"Down";
            case VK_LEFT: return L"Left";
            case VK_RIGHT: return L"Right";
            case VK_UP: return L"Up";
            case VK_END: return L"End";
            case VK_NAVIGATION_DOWN: return L"Pg Down";
            case VK_NAVIGATION_UP: return L"Pg Up";
            case VK_CAPITAL: return L"Caps";
            case VK_OEM_PERIOD: return L".";
            case VK_OEM_COMMA: return L",";
            case VK_OEM_1: return L";";
            case VK_OEM_2: return L"/";
            case VK_OEM_3: return L"";
            case VK_OEM_4: return L"[";
            case VK_OEM_5: return L"\\";
            case VK_OEM_6: return L"]";
            case VK_OEM_7: return L"'";
        }
        
        // the rest we'll try to map via winapi
        wchar_t buf[32]{};
        auto scan_code = MapVirtualKeyA(virtual_key_code, MAPVK_VK_TO_VSC);
        GetKeyNameTextW(scan_code << 16, buf, sizeof(buf));

        return buf;
    }
}

namespace events
{
    std::map<std::type_index, size_t> observers_number{};
}

namespace far3
{
    template<class T>
    static std::shared_ptr<T> make_sized_shared(size_t size)
    {
        struct free_deleter
        {
            void operator()(void *data) { free(data); }
        };

        return std::shared_ptr<T>((T*)malloc(size), free_deleter());
    }

    string get_plugin_version()
    {
        return std::format("{}.{}.{}.{}.{}",
            PLUGIN_VERSION.Major,
            PLUGIN_VERSION.Minor,
            PLUGIN_VERSION.Revision,
            PLUGIN_VERSION.Build,
            (int)PLUGIN_VERSION.Stage);
    }

    namespace dialogs
    {
        void flush_vbuffer()
        {
            // found some hack: https://api.farmanager.com/ru/dialogapi/dmsg/dn_drawdialogdone.html
            return config::ps_info.Text(0, 0, NULL, NULL);
        }

        intptr_t send(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
        {
            return config::ps_info.SendDlgMessage(hdlg, msg, param1, param2);
        }

        SMALL_RECT get_dialog_rect(HANDLE hdlg)
        {
            SMALL_RECT dlg_rect;
            send(hdlg, DM_GETDLGRECT, 0, &dlg_rect);
            return dlg_rect;
        }
        
        intptr_t enable_redraw(HANDLE hdlg, bool is_enable)
        {
            return send(hdlg, DM_ENABLEREDRAW, is_enable, 0);
        }
        
        intptr_t close(HANDLE hdlg)
        {
            return send(hdlg, DM_CLOSE, -1, 0);
        }
        
        intptr_t resize_dialog(HANDLE hdlg, SHORT width, SHORT height)
        {
            COORD coord{ width, height };
            return send(hdlg, DM_RESIZEDIALOG, 0, &coord);
        }
        
        intptr_t move_dialog_to(HANDLE hdlg, SHORT x, SHORT y)
        {
            COORD coord = { x, y };
            return send(hdlg, DM_MOVEDIALOG, 1, &coord);
        }

        intptr_t move_dialog_by(HANDLE hdlg, SHORT distance_x, SHORT distance_y)
        {
            COORD coord = { distance_x, distance_y };
            return send(hdlg, DM_MOVEDIALOG, 0, &coord);
        }
        
        intptr_t enable(HANDLE hdlg, int ctrl_id, bool is_enabled)
        {
            return send(hdlg, DM_ENABLE, ctrl_id, (void*)is_enabled);
        }
        
        bool is_enabled(HANDLE hdlg, int ctrl_id)
        {
            return send(hdlg, DM_ENABLE, ctrl_id, (void*)-1) == TRUE;
        }
        
        intptr_t set_focus(HANDLE hdlg, int ctrl_id)
        {
            return send(hdlg, DM_SETFOCUS, ctrl_id, 0);
        }
        
        intptr_t set_visible(HANDLE hdlg, int ctrl_id, bool is_visible)
        {
            return send(hdlg, DM_SHOWITEM, ctrl_id, (void*)is_visible);
        }
        
        bool is_visible(HANDLE hdlg, int ctrl_id)
        {
            return send(hdlg, DM_SHOWITEM, ctrl_id, (void*)-1) == TRUE;
        }

        intptr_t set_checked(HANDLE hdlg, int ctrl_id, bool is_checked)
        {
            auto param2 = is_checked ? BSTATE_CHECKED : BSTATE_UNCHECKED;
            return send(hdlg, DM_SETCHECK, ctrl_id, reinterpret_cast<void*>(param2));
        }

        bool is_checked(HANDLE hdlg, int ctrl_id)
        {
            return send(hdlg, DM_GETCHECK, ctrl_id, NULL) == BSTATE_CHECKED;
        }
        
        intptr_t set_text(HANDLE hdlg, int ctrl_id, const wstring &text)
        {
            return send(hdlg, DM_SETTEXTPTR, ctrl_id, (void*)text.c_str());
        }
        
        intptr_t set_text(HANDLE hdlg, int ctrl_id, const string &text)
        {
            return set_text(hdlg, ctrl_id, to_wstring(text));
        }

        wstring get_text(HANDLE hdlg, int ctrl_id)
        {
            return wstring((const wchar_t*)send(hdlg, DM_GETCONSTTEXTPTR, ctrl_id, NULL));
        }

        intptr_t resize_item(HANDLE hdlg, int ctrl_id, SMALL_RECT rect)
        {
            return send(hdlg, DM_SETITEMPOSITION, ctrl_id, &rect);
        }
        
        size_t get_list_current_pos(HANDLE hdlg, int ctrl_id)
        {
            return send(hdlg, DM_LISTGETCURPOS, ctrl_id, NULL);
        }

        intptr_t clear_list(HANDLE hdlg, int ctrl_id)
        {
            return send(hdlg, DM_LISTDELETE, ctrl_id, NULL);
        }
        
        intptr_t open_list(HANDLE hdlg, int ctrl_id, bool is_opened)
        {
            intptr_t param2 = is_opened ? TRUE : FALSE;
            return send(hdlg, DM_SETDROPDOWNOPENED, ctrl_id, (void*)param2);
        }
        
        intptr_t add_list_item(HANDLE hdlg, int ctrl_id, const wstring &label, int index,
                                void *data, size_t data_size, bool is_selected, LISTITEMFLAGS flags)
        {
            FarListItem item{ flags, label.c_str(), NULL, NULL };
            if (is_selected)
                item.Flags |= LIF_SELECTED;
                
            FarList list{ sizeof(FarList), 1, &item };
            auto r = send(hdlg, DM_LISTADD, ctrl_id, &list);
            
            if (data != nullptr)
            {
                FarListItemData item_data{ sizeof(FarListItemData), index, data_size, data };
                send(hdlg, DM_LISTSETDATA, ctrl_id, &item_data);
            }
            return r;
        }
        
        intptr_t update_list_item(HANDLE hdlg, int ctrl_id, const wstring &label, int index,
                                 void *data, size_t data_size, bool is_selected, LISTITEMFLAGS flags)
        {
            FarListItem item{ flags, label.c_str(), NULL, NULL };
            if (is_selected)
                item.Flags |= LIF_SELECTED;
            
            if (data != nullptr)
            {
                FarListUpdate item_data{ sizeof(FarListUpdate), index, item };
                return send(hdlg, DM_LISTUPDATE, ctrl_id, &item_data);
            }
            return FALSE;
        }

        template<>
        string get_list_item_data<string>(HANDLE hdlg, int ctrl_id, size_t item_idx)
        {
            auto item_data = send(hdlg, DM_LISTGETDATA, ctrl_id, (void*)item_idx);
            size_t item_data_size = send(hdlg, DM_LISTGETDATASIZE, ctrl_id, (void*)item_idx);
            return string(reinterpret_cast<const char*>(item_data), item_data_size);
        }

        template<>
        wstring get_list_item_data<wstring>(HANDLE hdlg, int ctrl_id, size_t item_idx)
        {
            auto item_data = send(hdlg, DM_LISTGETDATA, ctrl_id, (void*)item_idx);
            size_t item_data_size = send(hdlg, DM_LISTGETDATASIZE, ctrl_id, (void*)item_idx);
            return wstring(reinterpret_cast<const wchar_t*>(item_data), item_data_size);
        }
    }

    namespace panels
    {
        intptr_t control(HANDLE panel, FILE_CONTROL_COMMANDS cmd, intptr_t param1, void *param2)
        {
            return config::ps_info.PanelControl(panel, cmd, param1, param2);
        }

        intptr_t redraw(HANDLE panel, size_t current_item_idx, size_t top_item_idx)
        {
            PanelRedrawInfo info{ sizeof(PanelRedrawInfo), current_item_idx, top_item_idx };
            return control(panel, FCTL_REDRAWPANEL, 0, &info);
        }

        intptr_t redraw(HANDLE panel)
        {
            return control(panel, FCTL_REDRAWPANEL, 0, NULL);
        }
        
        intptr_t update(HANDLE panel, bool reset_selection)
        {
            return control(panel, FCTL_UPDATEPANEL, reset_selection ? 0 : 1);
        }
        
        bool is_active(HANDLE panel)
        {
            return control(panel, FCTL_ISACTIVEPANEL) == TRUE;
        }
        
        bool does_exist(HANDLE panel)
        {
            return control(panel, FCTL_CHECKPANELSEXIST) == TRUE;
        }
        
        intptr_t set_active(HANDLE panel)
        {
            return control(panel, FCTL_SETACTIVEPANEL);
        }
        
        intptr_t set_view_mode(HANDLE panel, size_t view_mode_idx)
        {
            return control(panel, FCTL_SETVIEWMODE, view_mode_idx, 0);
        }
        
        intptr_t set_sort_mode(HANDLE panel, OPENPANELINFO_SORTMODES sort_mode, bool is_desc)
        {
            auto r = control(panel, FCTL_SETSORTMODE, sort_mode, 0);
            control(panel, FCTL_SETSORTORDER, is_desc ? TRUE : FALSE, 0);
            return r;
        }
        
        std::shared_ptr<PluginPanelItem> get_current_item(HANDLE panel)
        {
            size_t size = control(panel, FCTL_GETCURRENTPANELITEM, 0, 0);
            if (auto ppi = make_sized_shared<PluginPanelItem>(size))
            {
                FarGetPluginPanelItem fgppi = { sizeof(FarGetPluginPanelItem), size, ppi.get() };
                control(panel, FCTL_GETCURRENTPANELITEM, 0, &fgppi);

                return ppi;
            }
            return nullptr;
        }
        
        bool select_item(HANDLE panel, size_t item_idx)
        {
            if (control(panel, FCTL_SETSELECTION, item_idx, (void*)TRUE) == TRUE)
            {
                redraw(panel);
                return true;
            }
            return false;
        }
        
        void clear_selection(HANDLE panel)
        {
            auto panel_info = get_info(panel);
            if (panel_info.SelectedItemsNumber > 0)
            {
                for (size_t i = 0; i < panel_info.SelectedItemsNumber; i++)
                    control(panel, FCTL_CLEARSELECTION, i, NULL);
                redraw(panel);
            }
        }
        
        intptr_t set_directory(HANDLE panel, const wstring &folder)
        {
            FarPanelDirectory dirInfo = {sizeof(FarPanelDirectory), folder.c_str(), nullptr, {0}, nullptr};
            return control(panel, FCTL_SETPANELDIRECTORY, 0, &dirInfo);
        }
        
        std::vector<std::shared_ptr<PluginPanelItem>> get_items(HANDLE panel, bool filter_selected)
        {
            std::vector<std::shared_ptr<PluginPanelItem>> result;

            auto panel_info = get_info(panel);

            FILE_CONTROL_COMMANDS cmd = FCTL_GETPANELITEM;
            size_t items_number = panel_info.ItemsNumber;

            if (filter_selected)
            {
                cmd = FCTL_GETSELECTEDPANELITEM;
                items_number = panel_info.SelectedItemsNumber;
            }

            for (size_t i = 0; i < items_number; i++)
            {
                size_t size = control(panel, cmd, i, 0);
                if (auto ppi = make_sized_shared<PluginPanelItem>(size))
                {
                    FarGetPluginPanelItem fgppi = { sizeof(FarGetPluginPanelItem), size, ppi.get() };
                    control(panel, cmd, i, &fgppi);

                    result.push_back(ppi);
                }
            }
            return result;
        }
        
        PanelInfo get_info(HANDLE panel)
        {
            PanelInfo pinfo;
            control(panel, FCTL_GETPANELINFO, 0, &pinfo);
            return pinfo;
        }
        
        std::shared_ptr<FarPanelDirectory> get_directory(HANDLE panel)
        {
            size_t size = control(panel, FCTL_GETPANELDIRECTORY, NULL, NULL);
            if (size == 0)
                return nullptr;

            if (auto pdir = make_sized_shared<FarPanelDirectory>(size))
            {
                control(panel, FCTL_GETPANELDIRECTORY, size, pdir.get());
                return pdir;
            }
            return nullptr;
        }
        
        void quit(HANDLE panel)
        {
            control(panel, FCTL_CLOSEPANEL, 0, NULL);
        }
    }
    
    namespace plugins
    {
        HANDLE get_handle()
        {
            return (HANDLE)config::ps_info.PluginsControl(
                INVALID_HANDLE_VALUE, PCTL_FINDPLUGIN, PFM_GUID, (void*)&MainGuid);
        }

        std::shared_ptr<FarGetPluginInformation> get_info()
        {
            if (auto plugin_handle = get_handle())
            {
                intptr_t data_size = config::ps_info.PluginsControl(
                    plugin_handle, PCTL_GETPLUGININFORMATION, NULL, NULL);
                    
                if (auto plugin_info = make_sized_shared<FarGetPluginInformation>(data_size))
                {
                    config::ps_info.PluginsControl(plugin_handle, PCTL_GETPLUGININFORMATION, data_size, plugin_info.get());
                    return plugin_info;
                }
            }
            return nullptr;
        }

        bool unload()
        {
            if (auto plugin_handle = get_handle())
                return config::ps_info.PluginsControl(plugin_handle, PCTL_UNLOADPLUGIN, NULL, NULL) == TRUE;
            return false;
        }
    }
    
    namespace actl
    {
        intptr_t redraw_all()
        {
            return config::ps_info.AdvControl(&MainGuid, ACTL_REDRAWALL, 0, 0);
        }

        HWND get_far_hwnd()
        {
            return (HWND)config::ps_info.AdvControl(&MainGuid, ACTL_GETFARHWND, 0, 0);
        }

        intptr_t quit(intptr_t exit_code)
        {
            return config::ps_info.AdvControl(&MainGuid, ACTL_QUIT, exit_code, 0);
        }

        intptr_t synchro(void *user_data)
        {
            return config::ps_info.AdvControl(&MainGuid, ACTL_SYNCHRO, 0, user_data);
        }

        SMALL_RECT get_far_rect()
        {
            SMALL_RECT rect;
            config::ps_info.AdvControl(&MainGuid, ACTL_GETFARRECT, 0, &rect);
            return rect;
        }

        bool is_wnd_in_focus()
        {
            return GetForegroundWindow() == get_far_hwnd();
        }
    }

    intptr_t show_far_error_dlg(int error_msg_id, const wstring &extra_message,
        int extra_button_msg_id, std::function<void(void)> extra_btn_handler)
    {
        bool has_extra_btn = extra_button_msg_id != -1;
    
        std::vector<const wchar_t*> msgs =
        {
            get_text(MFarMessageErrorTitle),
            get_text(error_msg_id),
        };

        // custom extram message to show as a second string on the dialog
        if (!extra_message.empty())
            msgs.push_back(extra_message.c_str());
        
        msgs.push_back(get_text(MOk)); // mandatory `OK` button

        if (has_extra_btn) // extra custom button
            msgs.push_back(get_text(extra_button_msg_id));

        auto res = config::ps_info.Message(
            &MainGuid, &FarMessageGuid, FMSG_WARNING, 0, &msgs[0], std::size(msgs), has_extra_btn ? 2 : 1);
        
        // activate given handler for a custom button
        if (has_extra_btn && res == 1 && extra_btn_handler != nullptr)
            extra_btn_handler();

        //log::global->debug("Showing far error message, {}", utils::to_string(utils::string_join(msgs, L",")));

        return res;
    }

    const wchar_t* get_text(int msg_id)
    {
        return config::ps_info.GetMsg(&MainGuid, msg_id);
    }

    namespace synchro_tasks
    {
        static tasks_queue queue;
        
        void push(tasks_queue::task_t task, const string &task_descr)
        {
            auto task_id = queue.push_task(task, task_descr);
            actl::synchro((void*)task_id);
        }
        
        void process(intptr_t task_id)
        {
            return queue.process_one(task_id);
        }

        void clear()
        {
            return queue.clear_tasks();
        }
    }
}

namespace log
{
    std::shared_ptr<spdlog::logger>
        global = nullptr,
        api  = nullptr,
        librespot = nullptr;

    static std::wstreambuf *wcout_old_buf = nullptr;

    class callback_wstreambuf : public std::wstreambuf
    {
    public:
        callback_wstreambuf(std::function<void(wchar_t const*, std::streamsize)> callback): callback(callback) {}
    protected:
        std::streamsize xsputn(char_type const* s, std::streamsize count)
        {
            callback(s, count);
            return count;
        }
    
    private:
        std::function<void(wchar_t const*, std::streamsize)> callback;
    };
    
    static void wcout_stream_handler(wchar_t const* data, std::streamsize count)
    {
        log::api->debug("[WinToast] {}", utils::to_string(data));
    }

    void init()
    {
        auto filepath = std::format(L"{}\\spotifar.log", get_logs_folder());

        // a default sink to the file 
        auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filepath, 23, 59, false, 3);
        
        if (!spdlog::get("global"))
        {
            global = std::make_shared<spdlog::logger>("global", daily_sink);
            spdlog::set_default_logger(global);
        }

        // specific logger for spotify api communication
        if (!spdlog::get("api"))
        {
            api = std::make_shared<spdlog::logger>("api", daily_sink);
            spdlog::register_logger(api);
        }

        // specific logger for librespot messages
        if (!spdlog::get("librespot"))
        {
            librespot = std::make_shared<spdlog::logger>("librespot", daily_sink);
            spdlog::register_logger(librespot);
        }

        #ifdef _DEBUG
            // for debugging in VS Code this sink helps seeing the messages in the Debug Console view
            auto msvc_debug_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
            spdlog::apply_all([&msvc_debug_sink](const auto &logger)
            {
                logger->sinks().push_back(msvc_debug_sink);
            });

            // WinToast library send debug messages to the std::wcout, this is the attempt to redirect
            // them to the log. It looks like working, but not sure about any corner cases
            static auto buf = callback_wstreambuf(wcout_stream_handler);
            wcout_old_buf = std::wcout.rdbuf(&buf);
        #endif
        
        global->info("Plugin logging system is initialized");

        // note: the librespot logging is initialized with process startup,
        // the change will take place only after restarting the plugin
        enable_verbose_logs(config::is_verbose_logging_enabled());
    }

    void fini()
    {
        #ifdef _DEBUG
            if (wcout_old_buf != nullptr)
                std::wcout.rdbuf(wcout_old_buf);
        #endif

        log::global->info("Shutting down logging subsystem\n\n");
        spdlog::shutdown();
    }
    
    void tick(const clock_t::duration &delta)
    {
        static clock_t::duration flush_period = 10s, accumulated_delta{};

        // flushing logs once per `period` time
        accumulated_delta += delta;
        if (accumulated_delta >= flush_period)
        {
            accumulated_delta = accumulated_delta % delta;
            spdlog::apply_all([](const auto &logger){ logger->flush(); });
        }
    }

    void enable_verbose_logs(bool is_verbose)
    {
        auto level = spdlog::level::info;

        if (is_verbose)
            level = spdlog::level::debug;

        spdlog::set_level(level);

        global->info("Logging level has been changed: {}", spdlog::level::to_string_view(level));
    }

    wstring get_logs_folder()
    {
        return std::format(L"{}\\logs\\", config::get_plugin_data_folder());
    }
}

wstring utf8_decode(const string &s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len);
    return out;
}

string utf8_encode(const wstring &ws)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &out[0], len, NULL, NULL);
    return out;
}

wstring to_wstring(const string &s)
{
    return wstring(s.begin(), s.end());
}

string to_string(const wstring &ws)
{
    #pragma warning(suppress: 4244)  
    return string(ws.begin(), ws.end());
}

wstring strip_invalid_filename_chars(const wstring &filename)
{
    static auto r = std::wregex(L"[\?\\\\/:*<>|\\.]");
    return std::regex_replace(filename, r, L"_");
}

string get_last_system_error()
{
    struct deleter
    {
        void operator()(void *data) {
            if (data != NULL) LocalFree(data);
        }
    };

    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0,
        NULL);

    // wrapping up the message pointer to free it up right after the result
    // is returned
    std::shared_ptr<const TCHAR[]> msg((LPTSTR)lpMsgBuf, deleter());
    
    return utils::to_string((LPCTSTR)lpMsgBuf);
}

clock_t::duration get_timestamp(const string &time_str)
{
    std::istringstream ss(time_str);

    clock_t::time_point time_point;
    std::chrono::from_stream(ss, "%Y-%m-%dT%H:%M:%S%Z", time_point);

    return time_point.time_since_epoch();
}

string trim(const string &s)
{
    size_t first = s.find_first_not_of(' ');
    if (string::npos == first)
        return s;
    
    size_t last = s.find_last_not_of(' ');
    return s.substr(first, (last - first + 1));
}

wstring trim(const wstring &s)
{
    size_t first = s.find_first_not_of(L' ');
    if (string::npos == first)
        return s;
    
    size_t last = s.find_last_not_of(L' ');
    return s.substr(first, (last - first + 1));
}


intptr_t tasks_queue::push_task(task_t task, const string &task_descr)
{
    static intptr_t task_id = 0;

    {
        std::lock_guard lock(guard);
        tasks[++task_id] = std::make_pair(task, task_descr);
    }

    return task_id;
}

void tasks_queue::process_one(intptr_t task_id)
{
    auto it = tasks.find(task_id);
    if (it == tasks.end())
        return log::global->error("There is no task registered with the given id, {}", task_id);

    execute_task(it->second.first);
    
    {
        std::lock_guard lock(guard);
        tasks.erase(it);
    }
}

void tasks_queue::process_all()
{
    for (auto &[task_id, task]: tasks)
        execute_task(task.first);
    
    {
        std::lock_guard lock(guard);
        tasks.clear();
    }
}

void tasks_queue::clear_tasks()
{
    if (tasks.size() > 0)
    {
        log::global->error("Unfinished tasks are stuck in the queue, {}", tasks.size());
        for (const auto &[task_id, task]: tasks)
            log::global->error(task.second);
    }
    
    {
        std::lock_guard lock(guard);
        tasks.clear();
    }
}

void tasks_queue::execute_task(task_t &task)
{
    try
    {
        task();
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error while processing a task: {}", ex.what());
    }
}

namespace http
{
    std::pair<string, string> split_url(const string &url)
    {
        static std::regex pattern("(^.*://[^/?:]+)?(/?.*$)");
        
        std::smatch match;
        if (std::regex_search(url, match, pattern) && match.size() > 2)
            return std::make_pair(match[1], match[2]);

        return std::make_pair(url, "");
    }

    bool is_success(const http::Result &res)
    {
        return res && is_success(res->status);
    }

    bool is_success(int status_code)
    {
        return status_code == OK_200 || status_code == NoContent_204 ||
            status_code == NotModified_304;
    }
    
    string get_status_message(const http::Result &res)
    {
        if (!res)
            return httplib::to_string(res.error());
    
        string message = "";

        // trying to get an additional error message in the API response body
        Document doc;
        if (!doc.Parse(res->body).HasParseError())
            if (doc.HasMember("error") && doc["error"].HasMember("message"))
                message = doc["error"]["message"].GetString();

        return std::format("status {}, {}. {}", res->status, httplib::status_message(res->status), message);
    }

    string trim_params(const string &url)
    {
        return url.substr(0, url.find("?"));
    }
    
    string trim_domain(const string &url)
    {
        return split_url(url).second;
    }

    void json_body_builder::object(scope_handler_t scope)
    {
        return object("", scope);
    }

    void json_body_builder::object(const string &key, scope_handler_t scope)
    {
        if (!key.empty())
            Key(key);
        
        StartObject();
        scope();
        EndObject();
    }
    
    template<>
    void json_body_builder::insert(string value)
    {
        String(value);
    }

    template<>
    void json_body_builder::insert(int value)
    {
        Int(value);
    }

    template<>
    void json_body_builder::insert(bool value)
    {
        Bool(value);
    }
    
    string dump_headers(const Headers &headers)
    {
        string s;
        char buf[BUFSIZ];

        for (auto it = headers.begin(); it != headers.end(); ++it)
        {
            const auto &x = *it;
            snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
            s += buf;
        }

        return s;
    }

    string dump_error(const Request &req, const Response &res)
    {
        std::stringstream ss, query;
        
        for (auto it = req.params.begin(); it != req.params.end(); ++it)
            query << std::format("{}{}={}", (it == req.params.begin()) ? '?' : '&', it->first, it->second);

        ss  << "An error occured while making an http request: "
            << std::format("{} {} {}", req.method, req.version, req.path) << query.str() << std::endl;

        //ss << dump_headers(req.headers);

        ss << std::endl << "A response received: " << std::endl;
        ss << std::format("{} {}", res.status, res.version) << std::endl;

        ss << dump_headers(res.headers) << std::endl;

        if (!res.body.empty())
            ss << res.body << std::endl;

        return ss.str();
    }
}

namespace json
{
    // string
    void from_json(const Value &j, string &result)
    {
        result = j.GetString();
    }

    void to_json(Value &j, const string &result, Allocator &allocator)
    {
        j.SetString(result, allocator);
    }

    // int
    void from_json(const Value &j, int &result)
    {
        result = j.GetInt();
    }

    void to_json(Value &j, const int &result, Allocator &allocator)
    {
        j.SetInt(result);
    }

    // size_t
    void from_json(const Value &j, size_t &result)
    {
        result = j.GetUint64();
    }

    void to_json(Value &j, const size_t &result, Allocator &allocator)
    {
        j.SetUint64(result);
    }

    // bool
    void from_json(const Value &j, bool &result)
    {
        result = j.GetBool();
    }

    void to_json(Value &j, const bool &result, Allocator &allocator)
    {
        j.SetBool(result);
    }

    void pretty_print(Value &v)
    {
        StringBuffer sb;
        Writer<StringBuffer> writer(sb);
        v.Accept(writer);
        log::global->debug(sb.GetString());
    }
}

} // namespace utils
} // namespace spotifar
