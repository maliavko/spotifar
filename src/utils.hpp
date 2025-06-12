#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include "stdafx.h"
#include "rapidjson/error/en.h"

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

/// @brief Converts a time string of `2025-03-20T21:32:45.384Z` format to a timestamp
clock_t::duration get_timestamp(const string &time_str);

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

/// @brief A string_join specialization for C-style arrays
template<typename E, typename S = std::basic_string<E>>
S string_join(const std::vector<const E*> parts, const E *delim)
{
    std::vector<S> str_parts;
    for (const E *str: parts)
        str_parts.push_back(str);

    return string_join(str_parts, delim);
}

/// @brief Splits given string `s` by the given delimiter `delim` to the parts
template<typename E, typename S = std::basic_string<E>>
std::vector<S> string_split(const S &s, const E delim)
{
    std::vector<S> result;
    std::basic_stringstream<E, std::char_traits<E>, std::allocator<E>> ss(s);
    S item;

    while (getline(ss, item, delim))
        result.push_back(item);

    return result;
}

/// @brief A string_split specialization for C strings
template<typename E, typename S = std::basic_string<E>>
std::vector<S> string_split(const E *s, const E delim)
{
    return string_split(S(s), delim);
}

/// @brief Returns a copy of a given string without trailing whitespaces
string trim(const string &s);

/// @brief Returns a copy of a given string without trailing whitespaces
wstring trim(const wstring &s);

/// @brief boost::hash_combine
inline std::size_t combine(std::size_t seed, std::size_t h) noexcept
{
    seed ^= h + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
    return seed;
}

HINSTANCE open_web_browser(const string &address);

class tasks_queue
{
public:
    using task_t = std::function<void(void)>;

    auto push_task(task_t task, const string &task_descr = "") -> intptr_t;
    void process_one(intptr_t task_id);
    void process_all();
    void clear_tasks();
protected:
    void execute_task(task_t &task);
private:
    // task_id -> pair<task_handler, task_str_description>
    std::unordered_map<intptr_t, std::pair<task_t, string>> tasks{};
    std::mutex guard;
};

namespace log
{
    extern std::shared_ptr<spdlog::logger> global, api, librespot;

    void init();

    void fini();

    void tick(const clock_t::duration &delta);

    void enable_verbose_logs(bool is_verbose);

    wstring get_logs_folder();
}

namespace keys
{
    static const int
        none = 0x00,
        a = 0x41,
        b = 0x42,
        c = 0x43,
        d = 0x44,
        e = 0x45,
        f = 0x46,
        g = 0x47,
        h = 0x48,
        i = 0x49,
        j = 0x4A,
        k = 0x4B,
        l = 0x4C,
        m = 0x4D,
        n = 0x4E,
        o = 0x4F,
        p = 0x50,
        q = 0x51,
        r = 0x52,
        s = 0x53,
        t = 0x54,
        u = 0x55,
        v = 0x56,
        w = 0x57,
        x = 0x58,
        y = 0x59,
        z = 0x5A,
        key_0 = 0x30,
        key_9 = 0x39;

    namespace mods
    {
        // all the modifiers are placed in the HIWORD
        static const int
            ctrl    = 0x10000,
            alt     = 0x20000,
            shift   = 0x40000;
    }

    /// @brief Whether the given virtual key is pressed at the moment
    bool is_pressed(int virtual_key);

    /// @brief Converts the given `input record` to the combined int, including a pressed
    /// key + all the modifiers, so it can be used later as key_a + mods::ctrl + mods::shift e.g.
    int make_combined(const KEY_EVENT_RECORD &kir);

    /// @brief Converts the given `FarKey` to the combined int, including a pressed
    /// key + all the modifiers, so it can be used later as key_a + mods::ctrl + mods::shift e.g.
    int make_combined(const FarKey &fkey);

    /// @brief Returns a beautiful user readable string of the key plus modifiers
    wstring combined_to_string(int combined_key);

    /// @brief The function `GetKeyNameTextW` works no great, for some corner cases
    /// it returns empty or error translation. Plus the text will depend on the 
    /// locale language selected by user in OS. The function tries to be not
    /// dependent on these problems
    wstring vk_to_string(WORD virtual_key_code);
}

namespace events
{
    extern std::map<std::type_index, size_t> observers_number;

    /// @brief Returns a status flag, whether the specified `P` observer
    /// protocol has any active listeners subscribed
    template<class P>
    static bool has_observers()
    {
        return observers_number[typeid(P)] > 0;
    }

    /// @brief A facade method for subscribing to ObserverManager bus events, provides additional
    /// functionality to track a number of actively subscribed observers.
    /// @param is_weak indicates the listeners, which do not increment number of observers of the
    /// events specified and has no impact on the result of the `has_observers` method's result
    template<class P, class T>
    void start_listening(T *o, bool is_weak = false)
    {
        ObserverManager::subscribe<P>(o);

        if (!is_weak)
            observers_number[typeid(P)]++;
    }

    template<class P, class T>
    void stop_listening(T *o, bool is_weak = false)
    {
        ObserverManager::unsubscribe<P>(o);

        if (!is_weak)
        {
            assert(observers_number[typeid(P)] > 0);
            observers_number[typeid(P)]--;
        }
    }
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
        void flush_vbuffer();
        auto send(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2) -> intptr_t;
        auto get_dialog_rect(HANDLE hdlg) -> SMALL_RECT;
        auto enable_redraw(HANDLE hdlg, bool is_enable) -> intptr_t;
        auto close(HANDLE hdlg) -> intptr_t;
        auto resize_dialog(HANDLE hdlg, SHORT width, SHORT height) -> intptr_t;
        auto move_dialog_to(HANDLE hdlg, SHORT x = -1, SHORT y = -1) -> intptr_t;
        auto move_dialog_by(HANDLE hdlg, SHORT distance_x, SHORT distance_y) -> intptr_t;

        template<class T>
        auto get_dlg_data(HANDLE hdlg) -> T*
        {
            return reinterpret_cast<T*>(send(hdlg, DM_GETDLGDATA, 0, 0));
        }

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

        template<> wstring get_list_item_data(HANDLE hdlg, int ctrl_id, size_t item_idx);

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
        auto update(HANDLE panel, bool reset_selection) -> intptr_t;
        bool is_active(HANDLE panel);
        bool does_exist(HANDLE panel);
        auto set_active(HANDLE panel) -> intptr_t;
        auto set_view_mode(HANDLE panel, size_t view_mode_idx) -> intptr_t;
        auto set_sort_mode(HANDLE panel, OPENPANELINFO_SORTMODES sort_mode, bool is_desc = false) -> intptr_t;
        auto get_info(HANDLE panel) -> PanelInfo;
        auto get_directory(HANDLE panel) -> std::shared_ptr<FarPanelDirectory>;
        void quit(HANDLE panel);
        auto get_current_item(HANDLE panel) -> std::shared_ptr<PluginPanelItem>;
        bool select_item(HANDLE panel, size_t item_idx);
        void clear_selection(HANDLE panel);

        /// @brief Sets the given `folder` path to the `panel`
        auto set_directory(HANDLE panel, const wstring &folder) -> intptr_t;

        /// @brief Returns the items, currently placed on the `panel`.
        /// @param filter_selected returns only selected items, in case there is no selected items - returns
        /// the one under cursor, or none if the itme under cursor is `..`
        auto get_items(HANDLE panel, bool filter_selected = false) -> std::vector<std::shared_ptr<PluginPanelItem>>;
    }

    /// @brief https://api.farmanager.com/ru/service_functions/pluginscontrol.html
    namespace plugins
    {
        auto get_handle() -> HANDLE;
        auto get_info() -> std::shared_ptr<FarGetPluginInformation>;
        auto unload() -> bool;
    }

    namespace actl
    {
        auto redraw_all() -> intptr_t;
        auto get_far_hwnd() -> HWND;
        auto quit(intptr_t exit_code) -> intptr_t;
        auto synchro(void *user_data) -> intptr_t;

        /// @brief Is Far window in focus or not 
        auto is_wnd_in_focus() -> bool;
    }
    
    /// @brief Localize given far string id
    const wchar_t* get_text(int msg_id);

    /// @brief The method helps to get a localized string with the given `msg_id` and
    /// format it with the given `args`
    template<typename... Args>
    wstring get_vtext(int msg_id, Args&&... args)
    {
        return std::vformat(get_text(msg_id), std::make_wformat_args(args...));
    }

    /// @brief ProcessSynchroEventW mechanism
    namespace synchro_tasks
    {
        /// @brief Push a task, to be executed in the plugin's main thread
        void push(tasks_queue::task_t task, const string &task_descr = "");

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
            const auto &task_descr = std::format("dispatch event task, {}", typeid(P).name());
            far3::synchro_tasks::push([method, args...] {
                ObserverManager::notify(method, args...);
            }, task_descr);
        }
    }

    /// @brief Show an error far message, shows a second line with an additional information
    /// if provided via `extra_message`
    /// @param error_msg_id main message msg_id
    /// @param extra_message extra message string to show as a second line
    /// @param extra_button_msg_id extra button msg_id
    /// @param extra_btn_handler the extra buttong custom handler
    intptr_t show_far_error_dlg(int error_msg_id, const wstring &extra_message = L"",
        int extra_button_msg_id = -1, std::function<void(void)> extra_btn_handler = nullptr);
}

namespace json
{
    using rapidjson::Document;
    using rapidjson::Value;
    using rapidjson::StringBuffer;
    using rapidjson::Writer;
    using rapidjson::SizeType;
    using rapidjson::Pointer;
    using rapidjson::PrettyWriter;
    using rapidjson::kObjectType;
    using rapidjson::kArrayType;
    using rapidjson::ParseResult;
    using Allocator = typename Document::AllocatorType;
    
    /// @brief string support for rapidjson parse/pack
    void from_json(const Value &j, string &result);
    void to_json(Value &j, const string &result, Allocator &allocator);
    
    /// @brief integer support for rapidjson parse/pack
    void from_json(const Value &j, int &result);
    void to_json(Value &j, const int &result, Allocator &allocator);
    
    /// @brief size_t support for rapidjson parse/pack
    void from_json(const Value &j, size_t &result);
    void to_json(Value &j, const size_t &result, Allocator &allocator);
    
    /// @brief bool support for json parse/pack
    void from_json(const Value &j, bool &result);
    void to_json(Value &j, const bool &result, Allocator &allocator);

    /// @brief vector support for rapidjson parse/pack
    /// @note the vector's type T should support packing as well
    template<class T>
    void from_json(const Value &j, std::vector<T> &result)
    {
        result.resize(j.Size());

        for (SizeType i = 0; i < j.Size(); i++)
            from_json(j[i], result[i]);
    }

    template<class T>
    void to_json(Value &result, const std::vector<T> &data, Allocator &allocator)
    {
        result = Value(kArrayType);

        for (const auto &item: data)
        {
            Value value;
            to_json(value, item, allocator);

            result.PushBack(value, allocator);
        }
    }

    /// @brief deque support for rapidjson parse/pack
    /// @note the vector's type T should support packing as well
    template<class T>
    void from_json(const Value &j, std::deque<T> &result)
    {
        result.resize(j.Size());

        for (SizeType i = 0; i < result.size(); i++)
            from_json(j[i], result[i]);
    }

    template<class T>
    void to_json(Value &result, const std::deque<T> &data, Allocator &allocator)
    {
        result = Value(kArrayType);

        for (const auto &item: data)
        {
            Value value;
            to_json(value, item, allocator);

            result.PushBack(value, allocator);
        }
    }

    /// @brief unordered_map support for rapidjson parse/pack
    /// @note the vector's type T should support packing as well
    template<class V>
    void from_json(const Value &j, std::unordered_map<string, V> &result)
    {
        result.reserve(j.MemberCount());

        for (Value::ConstMemberIterator itr = j.MemberBegin();
            itr != j.MemberEnd(); ++itr)
        {
            auto &value = result[itr->name.GetString()] = {};
            from_json(itr->value, value);
        }
    }

    template<class V>
    void to_json(Value &result, const std::unordered_map<string, V> &data,
        Allocator &allocator)
    {
        result = Value(kObjectType);

        for (const auto &[k, v]: data)
        {
            Value value;
            to_json(value, v, allocator);

            result.AddMember(Value(k, allocator), value, allocator);
        }
    }

    /// @brief Dumps a given serialization supporting object to string. Returns a shared_ptr to StringBuffer
    /// @tparam T `value` object type, must support jsong serialization (from_json/to_json methods)
    template<class T>
    auto dump(const T &value) -> std::shared_ptr<StringBuffer>
    {
        Document doc;
        to_json(doc, value, doc.GetAllocator());

        auto sb = std::make_shared<StringBuffer>();
        Writer<StringBuffer> writer(*sb);

        doc.Accept(writer);

        return sb;
    }
    
    /// @brief Parses a given `json` string into given object `value`. Raises an exception if the parsing
    /// ends with errors eaither while parsing string or reading the data from the valid json
    template<class T>
    void parse_to(const string &json, T &value)
    {
        Document doc;
        if (doc.Parse(json).HasParseError())
            throw std::runtime_error(GetParseError_En(doc.GetParseError()));
        
        try
        {
            from_json(doc, value);
        }
        catch (const std::exception &ex)
        {
            log::global->error(std::format(
                "There is an error parsing a json object: {}. Object type '{}', jsong string '{}'",
                ex.what(), typeid(T).name(), json));
            throw ex;
        }
    }

    void pretty_print(Value &doc);
}

namespace http
{
    using namespace json;
    using namespace httplib;

    /// @brief Returns whether the http request's result is successful
    /// (response code is 200, 204 or 304)
    bool is_success(const http::Result &res);

    /// @brief Returns whether the http request's result is successful
    /// (response code is 200, 204 or 304)
    /// @param status_code httplib::Result->status
    bool is_success(int status_code);

    /// @brief Returns a string message, representing a response result
    string get_status_message(const http::Result &res);
    
    /// @brief Splits and returns a given `url` as a two components pair: domain & path with parameters.
    /// If the url has only domain, so the second part of the pair will be empty, if only path - the first is empty
    std::pair<string, string> split_url(const string &url);

    /// @brief Returns a given `url` without postfixed added parameters
    string trim_params(const string &url);
    
    /// @brief Returns a given `url` without prefixed domain name; returns a whole `url` as is
    /// in case of an error
    string trim_domain(const string &url);
    
    /// @brief A static constant, representing the flag of a session-wide caching
    static const auto session = clock_t::duration(-1);
    
    /// @brief A class-helper for packing up json body for the
    /// http requests. A wrapper around rapidjson::Writer class
    class json_body_builder: public Writer<StringBuffer>
    {
    public:
        using scope_handler_t = std::function<void(void)>;
    public:
        json_body_builder(): Writer<StringBuffer>(sb) {}

        inline const char* str() const { return sb.GetString(); }
        
        void object(scope_handler_t scope);
        void object(const string &key, scope_handler_t scope);

        template<class T>
        void insert(T value);

        template<class T>
        void insert(const string &key, T value)
        {
            Key(key);
            insert(value);
        }

        template<class T>
        void insert(const string &key, std::initializer_list<T> value)
        {
            insert(key, std::vector(value.begin(), value.end()));
        }

        template<class T>
        void insert(const string &key, std::vector<T> value)
        {
            Key(key);
            StartArray();
            for (const auto &v: value)
                insert(v);
            EndArray();
        }

    private:
        StringBuffer sb;
    };

    string dump_headers(const Headers &headers);

    string dump_error(const Request &req, const Response &res);
} // namespace http

/// @brief https://stackoverflow.com/questions/28675727/using-crc32-algorithm-to-hash-string-at-compile-time
namespace crc32
{
    // Generate CRC lookup table
    template <unsigned c, int k = 8>
    struct f : f<((c & 1) ? 0xedb88320 : 0) ^ (c >> 1), k - 1> {};
    template <unsigned c> struct f<c, 0>{enum {value = c};};

    #define A(x) B(x) B(x + 128)
    #define B(x) C(x) C(x +  64)
    #define C(x) D(x) D(x +  32)
    #define D(x) E(x) E(x +  16)
    #define E(x) F(x) F(x +   8)
    #define F(x) G(x) G(x +   4)
    #define G(x) H(x) H(x +   2)
    #define H(x) I(x) I(x +   1)
    #define I(x) f<x>::value ,

    constexpr unsigned crc_table[] = { A(0) };

    // Constexpr implementation and helpers
    constexpr uint32_t crc32_impl(const uint8_t* p, size_t len, uint32_t crc) {
        return len ?
                crc32_impl(p+1,len-1,(crc>>8)^crc_table[(crc&0xFF)^*p])
                : crc;
    }

    constexpr uint32_t crc32(const uint8_t* data, size_t length) {
        return ~crc32_impl(data, length, ~0);
    }

    constexpr size_t strlen_c(const char* str) {
        return *str ? 1+strlen_c(str+1) : 0;
    }

    constexpr uint32_t WSID(const char* str) {
        return crc32((uint8_t*)str, strlen_c(str));
    }
}

} // namespace utils

// exposing `log` namespace to the 
namespace log = utils::log;

} // namespace spotifar


// global overload for possibility using FarKey in hash-maps
template<>
struct std::hash<FarKey>
{
    std::size_t operator()(const FarKey &fkey) const
    {
        std::size_t res = 0;
        spotifar::utils::combine(res, fkey.VirtualKeyCode);
        spotifar::utils::combine(res, fkey.ControlKeyState);
        return res;
    }
};

bool operator==(const FarKey &lhs, const FarKey &rhs);

#endif // UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8