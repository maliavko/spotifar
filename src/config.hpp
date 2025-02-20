#ifndef CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#define CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace config {

namespace log = utils::log;

extern PluginStartupInfo ps_info;
extern FarStandardFunctions fsf;

/// @brief global hotkeys ids
namespace hotkeys
{
    enum
    {
        play = 333,
        skip_next,
        skip_previous,
        seek_forward,
        seek_backward,
        volume_up,
        volume_down,
        last
    };
}

struct settings
{
    /// @brief { hotkey_id, std::pair(virtual key code, key modifiers) }
    typedef std::unordered_map<int, std::pair<WORD, WORD>> hotkeys_t;

    bool add_to_disk_menu;
    bool is_global_hotkeys_enabled;
    bool verbose_logging;
    int localhost_service_port;
    wstring spotify_client_id, spotify_client_secret;
    wstring plugin_startup_folder, plugin_data_folder;
    hotkeys_t hotkeys;
};

struct config_observer: public BaseObserverProtocol
{
    /// @brief The global hotkeys setting's state has been changed: on/off
    virtual void on_global_hotkeys_setting_changed(bool is_enabled) {};

    /// @brief The logging verbosity level has been changed
    virtual void on_logging_verbocity_changed(bool is_verbose) {};

    /// @brief The event is called if hotkey's key code or modifiers have changed
    virtual void on_global_hotkey_changed(settings::hotkeys_t changed_keys) {};
};

class settings_context
{
public:
    /// @param subkey a subkey name in the format of words, separated by slashes
    ///                 e.g. "subkey1", "subkey1/key2/key3"
    settings_context(const wstring &subkey);

    bool get_bool(const wstring &name, bool def);
    std::int64_t get_int64(const wstring &name, std::int64_t def);
    int get_int(const wstring &name, int def);
    const wstring get_wstr(const wstring &name, const wstring &def);
    string get_str(const wstring &name, const string &def);
    
    void set_bool(const wstring &name, bool value);
    void set_int64(const wstring &name, std::int64_t value);
    void set_int(const wstring &name, int value);
    void set_wstr(const wstring &name, const wstring &value);
    void set_str(const wstring &name, const string &value);

    /// @brief Delete the value from the settings with the given `name`
    bool delete_value(const wstring &name);

    /// @brief Clears all the current context's level subkey settings
    bool clear_subkey();

    /// @brief Prints out all the settings of the current context
    void trace_all();

    /// @brief Returns a reference to the current settings object
    settings& get_settings();

    /// @brief Compares the current state of all the settings with
    /// their initial state and fire the difference observer events,
    /// to notify all the listeners
    void fire_events();
private:
    PluginSettings ps;
    settings settings_copy; // used for comparing the settings with the changed ones
    const wstring subkey_path;
    size_t subkey_idx = 0;
};

/// @brief Returns the settings context, which provides the access 
/// to the plugin settings
/// @param subkey a settings subkey to get settings context for. Can be nested
///                 like "subkey1/subkey2/key3"
std::shared_ptr<settings_context> lock_settings(const wstring &subkey = L"");

/// @brief Initialize settings
void read(const PluginStartupInfo *info);

/// @brief Dump settings to the far settings file
void write();

/// @brief Returns the flag, indicating whether the plugin is added to the disk menu
bool is_added_to_disk_menu();

/// @brief Returns the flag, indicating whether the global hotkeys are enabled
bool is_global_hotkeys_enabled();

/// additional verbose logging enabled
bool is_verbose_logging_enabled();

/// @brief Returns the Spotify client id, set by user
string get_client_id();

/// @brief Returns the Spotify client secret, set by user
string get_client_secret();

/// @brief Returns the localhost service port, set by user
int get_localhost_port();

/// @brief The absolute folder path, containing plugin files
const wstring& get_plugin_launch_folder();

/// @brief The absolute folder path, pointer to the users AppData/spotify folder
const wstring& get_plugin_data_folder();

/// @brief Returning a pair(virtual key code, modifiers) pointer or nullptr
std::pair<WORD, WORD>* get_hotkey(int hotkey_id);


/// @brief An interface to the class, which provides functionality to write
/// and read data to the far settings
struct persistent_data_abstract
{
    virtual ~persistent_data_abstract() {};

    /// @brief Read from the storage and deserialize
    virtual void read(settings_context &ctx) = 0;

    /// @brief Serialize and write to the storage
    virtual void write(settings_context &ctx) = 0;

    /// @brief Clear the stored data in the storage
    virtual void clear(settings_context &ctx) = 0;
};

/// @brief A class, which provides the functionality to store and read data
/// from the far settings
/// @tparam T type of the data to be stored
template<typename T>
class persistent_data: public persistent_data_abstract
{
public:
    typedef typename T value_t;

    persistent_data(const wstring &storage_key);

    virtual void read(settings_context &ctx);
    virtual void write(settings_context &ctx);
    virtual void clear(settings_context &ctx);

    const value_t& get() const { return data; }
    void set(const value_t &d) { data = d; }

protected:
    /// @brief Implements the correct way to read and interpret data from the storage
    /// @param ctx settings context
    /// @param key storage key name to read from
    /// @param data output data reference to assigne the final result to
    virtual void read_from_storage(
        settings_context &ctx, const wstring &key, value_t &data) = 0;

    /// @brief Implements the correct way to write data to the storage
    /// @param ctx settings context
    /// @param key storage key name to write to
    /// @param data the data to write
    virtual void write_to_storage(
        settings_context &ctx, const wstring &key, const value_t &data) = 0;

private:
    const wstring storage_key;
    value_t data;
};

template<typename T>
persistent_data<T>::persistent_data(const wstring &storage_key): storage_key(storage_key) {}

template<typename T>
void persistent_data<T>::read(settings_context &ctx)
{
    try
    {
        read_from_storage(ctx, storage_key, data);
    }
    catch(const json::parse_error &e)
    {
        // in case of an error, just discard a stored data and drop an error message to log
        log::global->error("Cached value \"{}\" is broken, discarding. {}",
            utils::to_string(storage_key), e.what());
        clear(ctx);
    }
}

template<typename T>
void persistent_data<T>::write(settings_context &ctx)
{
    write_to_storage(ctx, storage_key, data);
}

template<typename T>
void persistent_data<T>::clear(settings_context &ctx)
{
    ctx.delete_value(storage_key);
}

} // namespace config
} // namespace spotifar

#endif //CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B