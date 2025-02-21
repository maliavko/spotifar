#include "utils.hpp"
#include "config.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"

namespace spotifar { namespace utils {

namespace far3
{
    string get_plugin_version()
    {
        return std::format("{}.{}.{}.{}.{}",
            PLUGIN_VERSION.Major,
            PLUGIN_VERSION.Minor,
            PLUGIN_VERSION.Revision,
            PLUGIN_VERSION.Build,
            (int)PLUGIN_VERSION.Stage
            );
    }

    namespace keys
    {
        bool is_pressed(int virtual_key)
        {
            return GetKeyState(virtual_key);
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

        SMALL_RECT get_rect(HANDLE hdlg)
        {
            SMALL_RECT dlg_rect;
            send(hdlg, DM_GETDLGRECT, 0, &dlg_rect);
            return dlg_rect;
        }
        
        intptr_t close(HANDLE hdlg)
        {
            return send(hdlg, DM_CLOSE, -1, 0);
        }
        
        intptr_t enable(HANDLE hdlg, int ctrl_id, bool is_enabled)
        {
            return send(hdlg, DM_ENABLE, ctrl_id, (void*)is_enabled);
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
            return wstring((const wchar_t *)send(hdlg, DM_GETCONSTTEXTPTR, ctrl_id, NULL));
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
        intptr_t redraw(HANDLE panel, size_t current_item_idx, size_t top_item_idx)
        {
            PanelRedrawInfo info{ sizeof(PanelRedrawInfo), current_item_idx, top_item_idx };
            return config::ps_info.PanelControl(panel, FCTL_REDRAWPANEL, 0, &info);
        }
        
        intptr_t update(HANDLE panel)
        {
            return config::ps_info.PanelControl(panel, FCTL_UPDATEPANEL, 0, 0);
        }
        
        intptr_t is_active(HANDLE panel)
        {
            return config::ps_info.PanelControl(panel, FCTL_ISACTIVEPANEL, 0, 0);
        }
        
        intptr_t does_exist(HANDLE panel)
        {
            return config::ps_info.PanelControl(panel, FCTL_CHECKPANELSEXIST, 0, 0);
        }
        
        intptr_t set_active(HANDLE panel)
        {
            return config::ps_info.PanelControl(panel, FCTL_SETACTIVEPANEL, 0, 0);
        }
        
        intptr_t get_current_item(HANDLE panel)
        {
            // TODO: unfinished
            // size_t size = config::ps_info.PanelControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, 0);
            // PluginPanelItem *PPI=(PluginPanelItem*)malloc(size);
            // if (PPI)
            // {
            //     FarGetPluginPanelItem FGPPI={sizeof(FarGetPluginPanelItem), size, PPI};
            //     config::ps_info.PanelControl(PANEL_ACTIVE,FCTL_GETCURRENTPANELITEM, 0, &FGPPI);
                
            //     const far_user_data* data = nullptr;
            //     if (PPI->UserData.Data != nullptr)
            //     {
            //         data = static_cast<const far_user_data*>(PPI->UserData.Data);
            //         spdlog::debug("current item {}", album::make_uri(data->id));
            //         api->start_playback(album::make_uri(data->id));
            //     }
                
            //     free(PPI);
            // }
            return 0;
        }
        
        intptr_t set_view_mode(HANDLE panel, size_t view_mode_idx)
        {
            return config::ps_info.PanelControl(panel, FCTL_SETVIEWMODE, view_mode_idx, 0);
        }
        
        intptr_t set_sort_mode(HANDLE panel, OPENPANELINFO_SORTMODES sort_mode, bool is_desc)
        {
            auto r = config::ps_info.PanelControl(panel, FCTL_SETSORTMODE, sort_mode, 0);
            config::ps_info.PanelControl(panel, FCTL_SETSORTORDER, (intptr_t)is_desc, 0);
            return r;
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
        auto err_msg = get_text(error_msg_id);
        const wchar_t* msgs[] = {
            get_text(MFarMessageErrorTitle),
            err_msg, extra_message.c_str(),
            get_text(MOk),
        };
        
        FARMESSAGEFLAGS flags = FMSG_WARNING;
        if (extra_message.empty() && GetLastError())  // if there's no error code, no need to show it in the dialog
            flags |= FMSG_ERRORTYPE;

        return config::ps_info.Message(&MainGuid, &FarMessageGuid, flags, 0, msgs, ARRAYSIZE(msgs), 1);
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

string trim(const string &s)
{
    size_t first = s.find_first_not_of(' ');
    if (string::npos == first)
    {
        return s;
    }
    size_t last = s.find_last_not_of(' ');
    return s.substr(first, (last - first + 1));
}

wstring trim(const wstring &s)
{
    size_t first = s.find_first_not_of(L' ');
    if (string::npos == first)
    {
        return s;
    }
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
