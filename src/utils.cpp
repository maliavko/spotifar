#include "utils.hpp"
#include "config.hpp"

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

namespace keys
{
    bool is_pressed(int virtual_key)
    {
        return GetKeyState(virtual_key) < 0;
    }

    int make_combined(const KEY_EVENT_RECORD &kir)
    {
        int key = static_cast<int>(kir.wVirtualKeyCode);
        const auto state = kir.dwControlKeyState;
        
        if (state & RIGHT_CTRL_PRESSED || state & LEFT_CTRL_PRESSED) key |= keys::mods::ctrl;
        if (state & RIGHT_ALT_PRESSED || state & LEFT_ALT_PRESSED) key |= keys::mods::alt;
        if (state & SHIFT_PRESSED) key |= keys::mods::shift;

        return key;
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
                                void *data, size_t data_size, bool is_selected)
        {
            FarListItem item{ LIF_NONE, label.c_str(), NULL, NULL };
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

        template<>
        string get_list_item_data<string>(HANDLE hdlg, int ctrl_id, size_t item_idx)
        {
            auto item_data = send(hdlg, DM_LISTGETDATA, ctrl_id, (void*)item_idx);
            size_t item_data_size = send(hdlg, DM_LISTGETDATASIZE, ctrl_id, (void*)item_idx);
            return string(reinterpret_cast<const char*>(item_data), item_data_size);
        }
    }

    namespace panels
    {
        struct free_deleter
        {
            void operator()(void *data) { free(data); }
        };

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
        
        intptr_t update(HANDLE panel)
        {
            return control(panel, FCTL_UPDATEPANEL);
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
            std::shared_ptr<PluginPanelItem> ppi((PluginPanelItem*)malloc(size), free_deleter());
            if (ppi)
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
                std::shared_ptr<PluginPanelItem> ppi((PluginPanelItem*)malloc(size), free_deleter());
                if (ppi)
                {
                    FarGetPluginPanelItem fgppi = { sizeof(FarGetPluginPanelItem), size, ppi.get() };
                    control(panel, cmd, i, &fgppi);
                    ppi->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;

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
    }

    intptr_t show_far_error_dlg(int error_msg_id, const wstring &extra_message)
    {
        // TODO: refactor dialog system
        auto err_msg = get_text(error_msg_id);
        const wchar_t* msgs[] = {
            get_text(MFarMessageErrorTitle),
            err_msg, extra_message.c_str(),
            get_text(MOk),
        };
        
        FARMESSAGEFLAGS flags = FMSG_WARNING;
        if (extra_message.empty() && GetLastError())  // if there's no error code, no need to show it in the dialog
            flags |= FMSG_ERRORTYPE;

        return config::ps_info.Message(&MainGuid, &FarMessageGuid, flags, 0, msgs, std::size(msgs), 1);
    }
    
    intptr_t show_far_error_dlg(int error_msg_id, const string &extra_message)
    {
        return show_far_error_dlg(error_msg_id, utils::to_wstring(extra_message));
    }

    const wchar_t* get_text(int msg_id)
    {
        return config::ps_info.GetMsg(&MainGuid, msg_id);
    }

    namespace synchro_tasks
    {
        static tasks_queue queue;
        
        void push(tasks_queue::task_t task)
        {
            auto task_id = queue.push_task(task);
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

    void init()
    {
        auto filepath = std::format(L"{}\\logs\\spotifar.log", config::get_plugin_data_folder());

        // a default sink to the file 
        auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filepath, 23, 59, false, 3);
        
        global = std::make_shared<spdlog::logger>("global", daily_sink);
        spdlog::set_default_logger(global);

        // specific logger for spotify api communication
        api = std::make_shared<spdlog::logger>("api", daily_sink);
        spdlog::register_logger(api);

        // specific logger for librespot messages
        librespot = std::make_shared<spdlog::logger>("librespot", daily_sink);
        spdlog::register_logger(librespot);

        #ifdef _DEBUG
            // for debugging in VS Code this sink helps seeing the messages in the Debug Console view
            auto msvc_debug_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
            spdlog::apply_all([&msvc_debug_sink](const auto &logger)
            {
                logger->sinks().push_back(msvc_debug_sink);
            });
        #endif
        
        global->info("Plugin logging system is initialized");

        // note: the librespot logging is initialized with process startup,
        // the change will take place only after restarting the plugin
        enable_verbose_logs(config::is_verbose_logging_enabled());
    }

    void fini()
    {
        log::global->info("Closing plugin\n\n");
        spdlog::shutdown();
    }

    void enable_verbose_logs(bool is_verbose)
    {
        auto level = spdlog::level::info;

        if (is_verbose)
            level = spdlog::level::debug;

        spdlog::set_level(level);

        global->info("Logging level has been changed: {}", spdlog::level::to_string_view(level));
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
    static auto r = std::wregex(L"[\?\\\\/:*<>|]");
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


intptr_t tasks_queue::push_task(task_t task)
{
    static intptr_t task_id = 0;
    tasks[++task_id] = task;
    return task_id;
}

void tasks_queue::process_one(intptr_t task_id)
{
    auto it = tasks.find(task_id);
    if (it == tasks.end())
        return log::global->error("There is no task registered with the given id, {}", task_id);

    execute_task(it->second);
    tasks.erase(it);
}

void tasks_queue::process_all()
{
    for (auto& [task_id, task]: tasks)
        execute_task(task);
    tasks.clear();
}

void tasks_queue::clear_tasks()
{
    if (tasks.size() > 0)
        log::global->error("Unexpected tasks are stuck in the queue, {}",
                            tasks.size());
    tasks.clear();
}

void tasks_queue::execute_task(task_t &task)
{
    try
    {
        task();
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error while processing task: {}",
                            ex.what());
    }
}

namespace http
{
    using namespace httplib;
    
    bool is_success(int response_code)
    {
        return (response_code == OK_200 || response_code == NoContent_204 ||
            response_code == NotModified_304);
    }
}



} // namespace utils
} // namespace spotifar
