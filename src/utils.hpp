#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include "stdafx.h"

namespace spotifar { namespace utils {

using clock_t = std::chrono::system_clock;

/// @brief Converts utf8 encoded string into wide-char one
wstring utf8_decode(const string &s);

/// @brief Converts wide-char string into utf8 encoded string
string utf8_encode(const wstring &ws);

/// @brief Bluntly converts char string into wide-char string
/// @note The function does not care about string encoding, all the multi-byte
/// stuff will be broken miserably
wstring to_wstring(const string &s);

/// @brief Bluntly converts char string into wide-char string
/// @note The function does not care about string encoding, all the multi-byte
/// stuff will be broken miserably
string to_string(const wstring &ws);

/// @brief Replaces impossible filename chars from the given string
/// with the underscore
wstring strip_invalid_filename_chars(const wstring &filename);

// TODO: making it thread safe
class tasks_queue
{
public:
    typedef std::function<void(void)> task_t;

    intptr_t push_task(task_t task);
    void process_one(intptr_t task_id);
    void process_all();
    void clear_tasks();
protected:
    void execute_task(task_t &task);
private:
    std::unordered_map<intptr_t, task_t> tasks{};
};

namespace log
{
    extern std::shared_ptr<spdlog::logger> global, api, librespot;

    void init();

    void fini();

    void enable_verbose_logs(bool is_verbose);
}

namespace far3
{
    /// @brief Far3 16 palette colors
    namespace colors
    {
        enum
        {
            black = 0,
            blue,
            green,
            cyan,
            red,
            purple,
            brown,
            gray,
            dgray,
            lblue,
            lgreen,
            lcyan,
            lred,
            lpurple,
            yellow,
            white,
        };
    }

    namespace keys
    {
        static const int
            none = 0x00,
            a = 0x41,
            d = 0x44,
            r = 0x52,
            s = 0x53,
            z = 0x5A,
            key_0 = 0x30,
            key_9 = 0x39;

        namespace mods
        {
            static const int
                ctrl = 0x100000,
                alt = 0x200000,
                shift = 0x400000;
        }
    }

    /// @brief Returns plugin version in the format "Major.Minor.Revision.Build.Stage"
    string get_plugin_version();

    int input_record_to_combined_key(const KEY_EVENT_RECORD &kir);

    namespace dialogs
    {
        void flush_vbuffer();
        intptr_t send(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2);
        intptr_t close(HANDLE hdlg);
        intptr_t enable(HANDLE hdlg, int ctrl_id, bool is_enabled);
        intptr_t set_checked(HANDLE hdlg, int ctrl_id, bool is_checked);
        bool is_checked(HANDLE hdlg, int ctrl_id);
        intptr_t set_text(HANDLE hdlg, int ctrl_id, const wstring &text);
        intptr_t set_text(HANDLE hdlg, int ctrl_id, const string &text);
        wstring get_text(HANDLE hdlg, int ctrl_id);
        intptr_t clear_list(HANDLE hdlg, int ctrl_id);
        size_t get_list_current_pos(HANDLE hdlg, int ctrl_id);
        intptr_t open_list(HANDLE hdlg, int ctrl_id, bool is_opened);
        intptr_t add_list_item(HANDLE hdlg, int ctrl_id, const wstring &label, int index,
                               void *data = nullptr, size_t data_size = 0, bool is_selected = false);

        /// @brief Get data from the list item
        /// @param hdlg dialog handle
        /// @param ctrl_id control id
        /// @param item_idx item index in the list
        /// @tparam T type of the data to be returned
        template<class T>
        T get_list_item_data(HANDLE hdlg, int ctrl_id, size_t item_idx)
        {
            auto item_data = dialogs::send(hdlg, DM_LISTGETDATA, ctrl_id, (void*)item_idx);
            size_t item_data_size = dialogs::send(hdlg, DM_LISTGETDATASIZE, ctrl_id, (void*)item_idx);
            return reinterpret_cast<T>(item_data);
        }
        template<> string get_list_item_data(HANDLE hdlg, int ctrl_id, size_t item_idx);
    }

    namespace panels
    {
        intptr_t redraw(HANDLE panel = PANEL_ACTIVE);
        intptr_t update(HANDLE panel = PANEL_ACTIVE);
        intptr_t is_active(HANDLE panel);
        intptr_t does_exist(HANDLE panel);
        intptr_t set_active(HANDLE panel);
        intptr_t get_current_item(HANDLE panel = PANEL_ACTIVE);
    }

    namespace actl
    {
        intptr_t redraw_all();
        HWND get_far_hwnd();
        intptr_t quit(intptr_t exit_code);
        intptr_t synchro(void *user_data);
    }
    
    /// @brief Localize given far string id
    const wchar_t* get_text(int msg_id);

    /// @brief ProcessSynchroEventW mechanism
    namespace synchro_tasks
    {
        /// @brief Push a task, to be executed in the plugin's main thread
        void push(tasks_queue::task_t task);

        /// @brief Execute a task with a given id
        void process(intptr_t task_id);

        /// @brief Clear the tasks queue
        void clear();
        
        /// @brief Fire an ObserverManager event in the context of a plugin's main thread
        /// @tparam P observer class
        /// @param method observer method to be called
        /// @param args arguments to be passed to the observer method
        template <class P, typename... MethodArgumentTypes, typename... ActualArgumentTypes>
        static void dispatch_event(void (P::*method)(MethodArgumentTypes...), ActualArgumentTypes... args)
        {
            far3::synchro_tasks::push([method, args...] {
                ObserverManager::notify(method, args...);
            });
        }
    }

    intptr_t show_far_error_dlg(int error_msg_id, const wstring &extra_message = L"");
    intptr_t show_far_error_dlg(int error_msg_id, const string &extra_message = "");
}

} // namespace utils
} // namespace spotifar

#endif // UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8