#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include "stdafx.h"
#include "config.hpp"

namespace spotifar { namespace utils {

using clock = std::chrono::system_clock;
using ms = std::chrono::milliseconds;
using SettingsCtx = config::SettingsContext;

/// @brief Converts utf8 encoded string into wide-char one
wstring utf8_decode(const string &s);

/// @brief Converts wide-char string into utf8 encoded string
string utf8_encode(const wstring &ws);

/// @brief Bluntly converts char string into wide-char string
wstring to_wstring(const string &s);

/// @brief Bluntly converts char string into wide-char string
/// @note The function does not care about string encoding, all the multi-byte
/// stuff will be broken miserably
string to_string(const wstring &ws);

/// @brief Replaces impossible filename chars from the given string
/// with the underscore
wstring strip_invalid_filename_chars(const wstring &filename);

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
    extern std::shared_ptr<spdlog::logger> global, api;

    void init();
    void fini();
}

namespace far3
{
    enum Colors16
    {
        CLR_BLACK = 0,
        CLR_BLUE,
        CLR_GREEN,
        CLR_CYAN,
        CLR_RED,
        CLR_PURPLE,
        CLR_BROWN,
        CLR_GRAY,
        CLR_DGRAY,
        CLR_LBLUE,
        CLR_LGREEN,
        CLR_LCYAN,
        CLR_LRED,
        CLR_LPURPLE,
        CLR_YELLOW,
        CLR_WHITE
    };

    static const int
        KEY_NONE = 0x00,
        KEY_A = 0x41,
        KEY_D = 0x44,
        KEY_R = 0x52,
        KEY_S = 0x53,
        KEY_CTRL = 0x100000,
        KEY_ALT = 0x200000,
        KEY_SHIFT = 0x400000;

    int input_record_to_combined_key(const KEY_EVENT_RECORD &kir);

    struct [[nodiscard]] NoRedraw
    {
        NoRedraw(HANDLE hdlg);
        ~NoRedraw();
        
        HANDLE hdlg;
        inline static std::mutex mutex{};
    };

    intptr_t show_far_error_dlg(int error_msg_id, const wstring &extra_message = L"");
    intptr_t show_far_error_dlg(int error_msg_id, const string &extra_message = "");
    
    intptr_t send_dlg_msg(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2);
    intptr_t close_dlg(HANDLE hdlg);
    intptr_t set_enabled(HANDLE hdlg, int ctrl_id, bool is_enabled);
    intptr_t set_checkbox(HANDLE hdlg, int ctrl_id, bool is_checked);
    bool get_checkbox(HANDLE hdlg, int ctrl_id);
    intptr_t set_textptr(HANDLE hdlg, int ctrl_id, const wstring &text);
    intptr_t set_textptr(HANDLE hdlg, int ctrl_id, const string &text);
    wstring get_textptr(HANDLE hdlg, int ctrl_id);
    intptr_t clear_list(HANDLE hdlg, int ctrl_id);
    size_t get_list_current_pos(HANDLE hdlg, int ctrl_id);
    intptr_t open_dropdown(HANDLE hdlg, int ctrl_id, bool is_opened);
    intptr_t add_list_item(HANDLE hdlg, int ctrl_id, const wstring &label, int index,
                           void *data = nullptr, size_t data_size = 0, bool is_selected = false);
    
    template<class DataType>
    DataType get_list_item_data(HANDLE hdlg, int ctrl_id, size_t item_idx)
    {
        auto item_data = send_dlg_msg(hdlg, DM_LISTGETDATA, ctrl_id, (void*)item_idx);
        size_t item_data_size = send_dlg_msg(hdlg, DM_LISTGETDATASIZE, ctrl_id, (void*)item_idx);
        return reinterpret_cast<DataType>(item_data);
    }
    template<> string get_list_item_data(HANDLE hdlg, int ctrl_id, size_t item_idx);
    
    /// @brief Localize given far string id 
    const wchar_t* get_msg(int msg_id);

    /// @brief ProcessSynchroEventW mechanism
    namespace synchro_tasks
    {
        void push(tasks_queue::task_t task);
        void process(intptr_t task_id);
        void clear();
    }


    struct IStorableData
    {
        virtual ~IStorableData() {};

        virtual void read(SettingsCtx &ctx) = 0;
        virtual void write(SettingsCtx &ctx) = 0;
        virtual void clear(SettingsCtx &ctx) = 0;
    };
    
    template<class T>
    class StorageValue: public IStorableData
    {
    public:
        typedef typename T ValueType;

    public:
        StorageValue(const wstring &storage_key);

        virtual void read(SettingsCtx &ctx);
        virtual void write(SettingsCtx &ctx);
        virtual void clear(SettingsCtx &ctx);

        const ValueType& get() const { return data; }
        void set(const ValueType &d) { data = d; }

    protected:
        virtual void read_from_settings(SettingsCtx &ctx, const wstring &key, ValueType &data) = 0;
        virtual void write_to_settings(SettingsCtx &ctx, const wstring &key, ValueType &data) = 0;

    private:
        const wstring storage_key;
        ValueType data;
    };
    
    template<class T>
    StorageValue<T>::StorageValue(const wstring &storage_key):
        storage_key(storage_key)
        {}

    template<class T>
    void StorageValue<T>::read(SettingsCtx &ctx)
    {
        try
        {
            read_from_settings(ctx, storage_key, data);
        }
        catch(const std::exception &e)
        {
            // in case of an error, just discard a stored data and drop an error message to log
            log::global->error("Cached value \"{}\" is broken, discarding. {}",
                utils::to_string(storage_key), e.what());
            clear(ctx);
        }
    }

    template<class T>
    void StorageValue<T>::write(SettingsCtx &ctx)
    {
        write_to_settings(ctx, storage_key, data);
    }

    template<class T>
    void StorageValue<T>::clear(SettingsCtx &ctx)
    {
        ctx.delete_value(storage_key);
    }
}

} // namespace utils
} // namespace spotifar

#endif //UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8