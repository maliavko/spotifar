#ifndef CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#define CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace config {

namespace json = utils::json;

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
        show_toast,
        last
    };
}

namespace playback
{
    namespace bitrate
    {
        inline static const string bps96 = "96";
        inline static const string bps160 = "160";
        inline static const string bps320 = "320";

        static const std::vector<string> all{ bps96, bps160, bps320 };

        /// @brief Checks whether the given `bps` is a valid bitrate value
        bool is_valid(const string &bps);
    }

    namespace format
    {
        // a default playback device Rodio supports only S16 and F32 formats
        inline static const string S16 = "S16";
        // inline static const string S24_3 = "S24_3";
        // inline static const string S24 = "S24";
        // inline static const string S32 = "S32";
        inline static const string F32 = "F32";
        // inline static const string F64 = "F64";

        static const std::vector<string> all{ S16, /*S24_3, S24, S32,*/ F32, /*F64*/ };

        /// @brief Checks whether the given format supports dithering
        bool does_support_dither(const string &fmt);

        /// @brief Checks whether the given `fmt` is a valid format value
        bool is_valid(const string &fmt);
    }

    namespace dither
    {
        inline static const string none = "none";
        inline static const string gpdf = "gpdf";
        inline static const string tpdf = "tpdf";
        inline static const string tpdf_hp = "tpdf_hp";

        static const std::vector<string> all{ none, gpdf, tpdf, tpdf_hp };

        /// @brief Checks whether the given `dither` is a valid dithering value
        bool is_valid(const string &dither);
    }

    namespace volume_ctrl
    {
        inline static const string cubic = "cubic";
        inline static const string fixed = "fixed";
        inline static const string linear = "linear";
        inline static const string log = "log";

        static const std::vector<string> all{ cubic, fixed, linear, log };

        /// @brief Checks whether the given `vctrl` is a valid volume control value
        bool is_valid(const string &vctrl);
    }
}

struct settings
{
    /// @brief A struct for holding view's persistent settings, stored
    /// and recovered from Far config file
    struct view_t
    {
        int sort_mode_idx; // views last selected sort mode (Ctrl+F1, F2 etc.)
        bool is_descending; // is sorting oder descending
        int view_mode; // view's view mode (Ctrl+1,2 etc.)
        
        friend void from_json(const json::Value &j, view_t &v);
        friend void to_json(json::Value &j, const view_t &v, json::Allocator &allocator);
    };
    
    /// @brief { view_uid, { sort mode idx, is order descending, panel view mode } }
    using views_t = std::unordered_map<string, view_t>;

    /// @brief { hotkey_id, std::pair(virtual key code, key modifiers) }
    using hotkeys_t = std::unordered_map<int, std::pair<WORD, WORD>>;

    /// @brief A struct for holding multiview's persistent settings, stored
    /// and recovered from Far config file
    struct multiview_t
    {
        size_t idx; // last selected view from the multiview's range
        
        friend void from_json(const json::Value &j, multiview_t &v);
        friend void to_json(json::Value &j, const multiview_t &v, json::Allocator &allocator);
    };

    using multiviews_t = std::unordered_map<string, multiview_t>;

    struct search_dialog_t
    {
        bool is_albums = true;
        bool is_artists = true;
        bool is_tracks = true;
        bool is_playlists = true;
        
        friend void from_json(const json::Value &j, search_dialog_t &v);
        friend void to_json(json::Value &j, const search_dialog_t &v, json::Allocator &allocator);
    };

    // general settings
    bool add_to_disk_menu;
    bool verbose_logging;

    // spotify settings
    wstring spotify_client_id;
    wstring spotify_client_secret;
    int localhost_service_port;

    // notifications
    bool track_changed_notification_enabled;
    bool is_circled_notification_image;

    // backend settings
    bool playback_backend_enabled;
    bool volume_normalisation_enabled;
    bool playback_autoplay_enabled;
    bool gapless_playback_enabled;
    bool playback_cache_enabled;
    string playback_bitrate;
    string playback_format;
    string playback_dither;
    string playback_volume_ctrl;
    int playback_initial_volume;

    // global hotkeys
    bool is_global_hotkeys_enabled;
    hotkeys_t hotkeys;

    // complimentary data
    views_t views;
    multiviews_t mviews;
    wstring plugin_startup_folder;
    wstring plugin_data_folder;
    search_dialog_t search_dialog;
};

struct config_observer: public BaseObserverProtocol
{
    /// @brief The global hotkeys setting's state has been changed: on/off
    virtual void on_global_hotkeys_setting_changed(bool is_enabled) {}

    /// @brief The logging verbosity level has been changed
    virtual void on_logging_verbocity_changed(bool is_verbose) {}

    /// @brief The event is called if hotkey's key code or modifiers have changed
    virtual void on_global_hotkey_changed(settings::hotkeys_t changed_keys) {}

    /// @brief The event is called when playback backend activation is changed
    virtual void on_playback_backend_setting_changed(bool is_enabled) {}

    /// @brief The event is called when any of the backend configuration has changed
    virtual void on_playback_backend_configuration_changed() {}
};

class settings_context
{
public:
    /// @param subkey a subkey name in the format of words, separated by slashes
    ///                 e.g. "subkey1", "subkey1/key2/key3"
    settings_context(const wstring &subkey);

    auto get_bool(const wstring &name, bool def) -> bool;
    auto get_int64(const wstring &name, std::int64_t def) -> std::int64_t;
    auto get_int(const wstring &name, int def) -> int;
    auto get_wstr(const wstring &name, const wstring &def) -> wstring;
    auto get_str(const wstring &name, const string &def, std::function<bool(const string&)> validator = nullptr) -> string;
    
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
auto lock_settings(const wstring &subkey = L"") -> std::shared_ptr<settings_context>;

/// @brief Initialize settings
void read(const PluginStartupInfo *info);

/// @brief Dump settings to the far settings file
void write();

/// @brief The method is used to prepare config system for the next plugin launch
void cleanup();

/// @brief Returns the flag, indicating whether the plugin is added to the disk menu
bool is_added_to_disk_menu();

/// @brief Returns the flag, indicating whether the global hotkeys are enabled
bool is_global_hotkeys_enabled();

/// additional verbose logging enabled
bool is_verbose_logging_enabled();

/// @brief Returns the Spotify client id, set by user
auto get_client_id() -> string;

/// @brief Returns the Spotify client secret, set by user
auto get_client_secret() -> string;

/// @brief Returns the localhost service port, set by user
auto get_localhost_port() -> int;

/// @brief The absolute folder path, containing plugin files
auto get_plugin_launch_folder() -> const wstring&;

/// @brief The absolute folder path, pointer to the users AppData/spotify folder
auto get_plugin_data_folder() -> const wstring&;

/// @brief Returning a pair(virtual key code, modifiers) pointer or nullptr
auto get_hotkey(int hotkey_id) -> const std::pair<WORD, WORD>*;

/// @brief Returns persistent view settings by its `view_uid`. In case there is no view
/// settings cached, returns `def` value
auto get_view_settings(const string &view_uid, const settings::view_t &def) -> settings::view_t*;

/// @brief Returns a pointer to multiview's settings object by given `mview_id`
auto get_multiview_settings(const string &mview_id, const settings::multiview_t &def) -> settings::multiview_t*;

/// @brief A flag whether a librespot playback is active and should
/// be launched while starting the plugin
/// https://github.com/librespot-org/librespot/wiki/Options
auto is_playback_backend_enabled() -> bool;

/// https://github.com/librespot-org/librespot/wiki/Options
auto is_playback_normalisation_enabled() -> bool;

/// @brief Whether Spotify should continue playing similar tracks
/// after the playing context is ended
/// https://github.com/librespot-org/librespot/wiki/Options
auto is_playback_autoplay_enabled() -> bool;

/// @brief Shortened gap between tracks while playing one after another
/// https://github.com/librespot-org/librespot/wiki/Options
auto is_gapless_playback_enabled() -> bool;

/// @brief Whether the audio is cached while playing
/// https://github.com/librespot-org/librespot/wiki/Options
auto is_playback_cache_enabled() -> bool;

/// @brief One of the config::playback::bitrate::all
/// https://github.com/librespot-org/librespot/wiki/Options
auto get_playback_bitrate() -> string;

/// @brief One of the config::playback::format::all
/// https://github.com/librespot-org/librespot/wiki/Options
auto get_playback_format() -> string;

/// @brief One of the config::playback::dither::all
/// https://github.com/librespot-org/librespot/wiki/Options
auto get_playback_dither() -> string;

/// @brief Volume control interpolation method
/// https://github.com/librespot-org/librespot/wiki/Options
auto get_playback_volume_ctrl() -> string;

/// https://github.com/librespot-org/librespot/wiki/Options
auto get_playback_initial_volume() -> size_t;

/// @brief Whether to show Windows pop-up notification when the track is changed
bool is_track_changed_notification_enabled();

/// @brief THe image on the notifications will be squared or circled
bool is_notification_image_circled();


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
    using value_t = typename T;

    persistent_data(const wstring &storage_key);

    virtual void read(settings_context &ctx);
    virtual void write(settings_context &ctx);
    virtual void clear(settings_context &ctx);

    auto get() const -> const value_t& { return data; }
    auto get() -> value_t& { return data; }
    auto extract() -> value_t&& { return std::move(data); }
    void set(value_t &&d) { data = std::move(d); }
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
    catch (const std::exception &e)
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