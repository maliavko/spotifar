#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include "stdafx.h"

template<>
struct std::hash<FarKey>
{
    std::size_t operator()(const FarKey &fkey) const
    {
        std::size_t res = 0;
        nlohmann::detail::combine(res, fkey.VirtualKeyCode);
        nlohmann::detail::combine(res, fkey.ControlKeyState);
        return res;
    }
};

bool operator==(const FarKey &lhs, const FarKey &rhs);

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

/// @brief Returns the message of GetLastError function
string get_last_system_error();

/// @brief Join a vector of string into one, using given `delimeter`
/// @param parts string parts
/// @param delim delimeter
template<typename E, typename S = std::basic_string<E>>
S string_join(const std::vector<S> &parts, const E *delim)
{
    std::basic_ostringstream<E, std::char_traits<E>, std::allocator<E>> os;
    auto b = parts.begin(), e = parts.end();

    if (b != e)
    {
        std::copy(b, prev(e), std::ostream_iterator<S, E>(os, delim));
        b = prev(e);
    }
    if (b != e)
        os << *b;

    return os.str();
}

/// @brief Returns a copy of a given string without trailing whitespaces
string trim(const string &s);

/// @brief Returns a copy of a given string without trailing whitespaces
wstring trim(const wstring &s);


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

namespace keys
{
    static const int
        none = 0x00,
        a = 0x41,
        d = 0x44,
        q = 0x51,
        r = 0x52,
        s = 0x53,
        z = 0x5A,
        key_0 = 0x30,
        key_9 = 0x39;

    namespace mods
    {
        static const int
            ctrl    = 0x100000,
            alt     = 0x200000,
            shift   = 0x400000;
    }

    /// @brief Whether the given virtual key is pressed at the moment
    bool is_pressed(int virtual_key);

    /// @brief Converts the given input record to the combined int, including a pressed
    /// key + all the modifiers, so it can be used later as key_a + mods::ctrl + mods::shift e.g.
    int make_combined(const KEY_EVENT_RECORD &kir);

    /// @brief Returns a beautiful user readable string of the key plus modifiers
    wstring combined_to_string(int combined_key);

    /// @brief The function `GetKeyNameTextW` works no great, for some corner cases
    /// it returns empty or error translation. Plus the text will depend on the 
    /// locale language selected by user in OS. The function tries to be not
    /// dependent on these problems
    wstring vk_to_string(WORD virtual_key_code);
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

    /// @brief Returns plugin version in the format "Major.Minor.Revision.Build.Stage"
    string get_plugin_version();

    namespace dialogs
    {
        // dialog
        auto flush_vbuffer() -> void;
        auto send(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2) -> intptr_t;
        auto get_dialog_rect(HANDLE hdlg) -> SMALL_RECT;
        auto enable_redraw(HANDLE hdlg, bool is_enable) -> intptr_t;
        auto close(HANDLE hdlg) -> intptr_t;
        auto resize_dialog(HANDLE hdlg, SHORT width, SHORT height) -> intptr_t;
        auto move_dialog_to(HANDLE hdlg, SHORT x = -1, SHORT y = -1) -> intptr_t;
        auto move_dialog_by(HANDLE hdlg, SHORT distance_x, SHORT distance_y) -> intptr_t;

        // controls
        auto enable(HANDLE hdlg, int ctrl_id, bool is_enabled) -> intptr_t;
        auto is_enabled(HANDLE hdlg, int ctrl_id) -> bool;
        auto set_focus(HANDLE hdlg, int ctrl_id) -> intptr_t;
        auto set_visible(HANDLE hdlg, int ctrl_id, bool is_visible) -> intptr_t;
        auto is_visible(HANDLE hdlg, int ctrl_id) -> bool;
        auto set_checked(HANDLE hdlg, int ctrl_id, bool is_checked) -> intptr_t;
        auto is_checked(HANDLE hdlg, int ctrl_id) -> bool;
        auto set_text(HANDLE hdlg, int ctrl_id, const wstring &text) -> intptr_t;
        auto set_text(HANDLE hdlg, int ctrl_id, const string &text) -> intptr_t;
        auto get_text(HANDLE hdlg, int ctrl_id) -> wstring;
        auto resize_item(HANDLE hdlg, int ctrl_id, SMALL_RECT rect) -> intptr_t;
        auto clear_list(HANDLE hdlg, int ctrl_id) -> intptr_t;
        auto get_list_current_pos(HANDLE hdlg, int ctrl_id) -> size_t;
        auto open_list(HANDLE hdlg, int ctrl_id, bool is_opened) -> intptr_t;
        auto add_list_item(HANDLE hdlg, int ctrl_id, const wstring &label, int index,
                           void *data = nullptr, size_t data_size = 0, bool is_selected = false) -> intptr_t;

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

        template<class T>
        T get_list_current_item_data(HANDLE hdlg, int ctrl_id)
        {
            size_t pos = get_list_current_pos(hdlg, ctrl_id);
            return get_list_item_data<T>(hdlg, ctrl_id, pos);
        }
    }

    namespace panels
    {
        auto control(HANDLE panel, FILE_CONTROL_COMMANDS cmd, intptr_t param1 = 0, void *param2 = nullptr) -> intptr_t;
        auto redraw(HANDLE panel, size_t current_item_idx, size_t top_item_idx) -> intptr_t;
        auto redraw(HANDLE panel) -> intptr_t;
        auto update(HANDLE panel) -> intptr_t;
        auto is_active(HANDLE panel) -> bool;
        auto does_exist(HANDLE panel) -> bool;
        auto set_active(HANDLE panel) -> intptr_t;
        auto set_view_mode(HANDLE panel, size_t view_mode_idx) -> intptr_t;
        auto set_sort_mode(HANDLE panel, OPENPANELINFO_SORTMODES sort_mode, bool is_desc = false) -> intptr_t;
        auto get_info(HANDLE panel) -> PanelInfo;
        auto get_current_item(HANDLE panel) -> std::shared_ptr<PluginPanelItem>;

        /// @brief Returns the items, currently placed on the `panel`.
        /// @param filter_selected returns only selected items, in case there is no selected items - returns
        /// the one under cursor, or none if the itme under cursor is `..`
        auto get_items(HANDLE panel, bool filter_selected = false) -> std::vector<std::shared_ptr<PluginPanelItem>>;
    }   

    namespace actl
    {
        auto redraw_all() -> intptr_t;
        auto get_far_hwnd() -> HWND;
        auto quit(intptr_t exit_code) -> intptr_t;
        auto synchro(void *user_data) -> intptr_t;
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

namespace http
{
    bool is_success(int response_code);
    
    /// @brief A static constant, representing the flag of a session-wide caching
    static const auto session = clock_t::duration(-1);
}

} // namespace utils
} // namespace spotifar

#endif // UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8