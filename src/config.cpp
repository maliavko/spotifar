#include "config.hpp"
#include "utils.hpp"

namespace spotifar { namespace config {

using utils::far3::synchro_tasks::dispatch_event;

namespace playback
{
    namespace bitrate
    {
        bool is_valid(const string &bps)
        {
            return std::find(all.begin(), all.end(), bps) != all.end();
        }
    }

    namespace format
    {
        bool does_support_dither(const string &fmt)
        {
            return fmt == S16 || fmt == S24_3 || fmt == S24;
        }
        
        bool is_valid(const string &fmt)
        {
            return std::find(all.begin(), all.end(), fmt) != all.end();
        }
    }

    namespace dither
    {
        bool is_valid(const string &dither)
        {
            return std::find(all.begin(), all.end(), dither) != all.end();
        }
    }

    namespace volume_ctrl
    {
        bool is_valid(const string &vctrl)
        {
            return std::find(all.begin(), all.end(), vctrl) != all.end();
        }
    }
}

static const wchar_t
    *add_to_disk_menu_opt = L"AddToDisksMenu",
    *activate_global_hotkeys_opt = L"ActivateGlobalHotkeys",
    *verbose_logging_enabled_opt = L"EnableVerboseLogging",
    *views_settings_opt = L"ViewsSettings",
    *spotify_client_id_opt = L"SpotifyClientID",
    *spotify_client_secret_opt = L"SpotifyClientSecret",
    *localhost_service_port_opt = L"LocalhostServicePort",
    *playback_backend_enabled_opt = L"PlaybackBackendEnabled",
    *volume_normalisation_enabled_opt = L"VolumeNormalisationEnabled",
    *playback_autoplay_enabled_opt = L"PlaybackAutoplayEnabled",
    *gapless_playback_enabled_opt = L"GaplessPlaybackEnabled",
    *playback_cache_enabled_opt = L"PlaybackCacheEnabled",
    *playback_bitrate_opt = L"PlaybackBitrate",
    *playback_format_opt = L"PlaybackFormat",
    *playback_dither_opt = L"PlaybackDither",
    *playback_volume_ctrl_opt = L"PlaybackVolumeCtrl",
    *playback_initial_volume_opt = L"PlaybackInitialVolume",
    *track_changed_notification_enabled_opt = L"TrackChangedNotificationEnabled",
    *is_circled_notification_image_opt = L"IsNotificationImageCircled";

PluginStartupInfo ps_info;
FarStandardFunctions fsf;
settings _settings;

static wstring get_plugin_launch_folder(const struct PluginStartupInfo *info)
{
    return std::filesystem::path(info->ModuleName).parent_path().wstring();
}
static wstring get_user_app_data_folder()
{
    PWSTR app_data_path = NULL;
    HRESULT hres = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &app_data_path);

    // at first, we are trying to create a logs folder in users home directory,
    // if not possible, trying plugins home directory
    if (SUCCEEDED(hres))
        return std::format(L"{}\\spotifar", app_data_path);
    
    return config::get_plugin_launch_folder();
}

void from_json(const json::Value &j, settings::view_t &v)
{
    v.sort_mode_idx = j["sort_mode_idx"].GetInt();
    v.is_descending = j["is_descending"].GetBool();
    v.view_mode = j["view_mode"].GetInt();
}

void to_json(json::Value &result, const settings::view_t &v, json::Allocator &allocator)
{
    result = json::Value(json::kObjectType);

    result.AddMember("sort_mode_idx", json::Value(v.sort_mode_idx), allocator);
    result.AddMember("is_descending", json::Value(v.is_descending), allocator);
    result.AddMember("view_mode", json::Value(v.view_mode), allocator);
}

settings_context::settings_context(const wstring &subkey):
    ps(MainGuid, ps_info.SettingsControl), settings_copy(_settings), subkey_path(subkey)
{
    static std::wregex pattern(L"[^/]+");

    auto begin = std::wsregex_iterator{ subkey.begin(), subkey.end(), pattern };
    auto end = std::wsregex_iterator();

    for (auto i = begin; i != end; ++i)
    {
        ps.CreateSubKey(subkey_idx, i->str().c_str());
        subkey_idx++;
    }
}

void settings_context::fire_events()
{
    if (_settings.is_global_hotkeys_enabled != settings_copy.is_global_hotkeys_enabled)
        dispatch_event(&config_observer::on_global_hotkeys_setting_changed, _settings.is_global_hotkeys_enabled);

    settings::hotkeys_t changed_keys;
    for (const auto &[hotkey_id, hotkey]: _settings.hotkeys)
    {
        auto &old_hotkey = settings_copy.hotkeys[hotkey_id];
        if (hotkey.first != old_hotkey.first || hotkey.second != old_hotkey.second)
            changed_keys[hotkey_id] = hotkey;
    }
    
    if (changed_keys.size()) // send only if some keys have beeen changed
        dispatch_event(&config_observer::on_global_hotkey_changed, changed_keys);
    
    if (_settings.verbose_logging != settings_copy.verbose_logging)
        dispatch_event(&config_observer::on_logging_verbocity_changed, _settings.verbose_logging);

    // any playback's configuration changes require librespot process to restart, at any moment we send
    // only one event: either global activation status changed or any configuration settings changed
    if (_settings.playback_backend_enabled != settings_copy.playback_backend_enabled)
        dispatch_event(&config_observer::on_playback_backend_setting_changed, _settings.playback_backend_enabled);
    else if (
        (_settings.volume_normalisation_enabled != settings_copy.volume_normalisation_enabled) ||
        (_settings.playback_autoplay_enabled != settings_copy.playback_autoplay_enabled) ||
        (_settings.gapless_playback_enabled != settings_copy.gapless_playback_enabled) ||
        (_settings.playback_cache_enabled != settings_copy.playback_cache_enabled) ||
        (_settings.playback_bitrate != settings_copy.playback_bitrate) ||
        (_settings.playback_format != settings_copy.playback_format) ||
        (_settings.playback_dither != settings_copy.playback_dither) ||
        (_settings.playback_volume_ctrl != settings_copy.playback_volume_ctrl) ||
        (_settings.playback_initial_volume != settings_copy.playback_initial_volume)
    )
        dispatch_event(&config_observer::on_playback_backend_configuration_changed);

    settings_copy = _settings;
}

bool settings_context::get_bool(const wstring &name, bool def)
{
    return ps.Get(subkey_idx, name.c_str(), def);
}

std::int64_t settings_context::get_int64(const wstring &name, std::int64_t def)
{
    return ps.Get(subkey_idx, name.c_str(), def);
}

int settings_context::get_int(const wstring &name, int def)
{
    return ps.Get(subkey_idx, name.c_str(), def);
}

wstring settings_context::get_wstr(const wstring &name, const wstring &def)
{
    return ps.Get(subkey_idx, name.c_str(), def.c_str());
}

string settings_context::get_str(const wstring &name, const string &def)
{
    return utils::to_string(get_wstr(name, utils::to_wstring(def)));
}

void settings_context::set_bool(const wstring &name, bool value)
{
    ps.Set(subkey_idx, name.c_str(), value);
}

void settings_context::set_int64(const wstring &name, std::int64_t value)
{
    ps.Set(subkey_idx, name.c_str(), value);
}

void settings_context::set_int(const wstring &name, int value)
{
    ps.Set(subkey_idx, name.c_str(), value);
}

void settings_context::set_wstr(const wstring &name, const wstring &value)
{
    ps.Set(subkey_idx, name.c_str(), value.c_str());
}

void settings_context::set_str(const wstring &name, const string &value)
{
    set_wstr(name, utils::to_wstring(value));
}

bool settings_context::delete_value(const wstring& name)
{
    return ps.DeleteValue(subkey_idx, name.c_str());
}

bool settings_context::clear_subkey()
{
    return ps.DeleteSubKey(subkey_idx);
}

void settings_context::trace_all()
{
    FarSettingsEnum s;
    ps.Enum(subkey_idx, &s);

    log::global->debug("Tracing all the stored settings for \"{}\", count {}",
        utils::to_string(subkey_path), s.Count);

    for (size_t i = 0; i < s.Count; i++)
    {
        const auto item = s.Items[i];
        
        string type_name = "unknown";
        switch (item.Type)
        {
            case FST_UNKNOWN: type_name = "unknown"; break;
            case FST_SUBKEY: type_name = "subkey"; break;
            case FST_QWORD: type_name = "qword"; break;
            case FST_STRING: type_name = "string"; break;
            case FST_DATA: type_name = "userdata"; break;
        }

        log::global->debug("Name: {}, type: {}", utils::to_string(item.Name), type_name);
    }
}

settings& settings_context::get_settings()
{
    return _settings;
}

std::shared_ptr<settings_context> lock_settings(const wstring &subkey)
{
    return std::make_shared<settings_context>(subkey);
}

static wstring get_hotkey_node_name(int key)
{
    return std::format(L"hotkey_{}", key);
}

void read(const PluginStartupInfo *info)
{
    ps_info = *info;
    fsf = *info->FSF;
    ps_info.FSF = &fsf;

    auto ctx = lock_settings();

    // general
    _settings.add_to_disk_menu = ctx->get_bool(add_to_disk_menu_opt, true);
    _settings.verbose_logging = ctx->get_bool(verbose_logging_enabled_opt, false);

    // spotify
    _settings.spotify_client_id = ctx->get_wstr(spotify_client_id_opt, L"");
    _settings.spotify_client_secret = ctx->get_wstr(spotify_client_secret_opt, L"");
    _settings.localhost_service_port = ctx->get_int(localhost_service_port_opt, 5050);

    // notifications
    _settings.track_changed_notification_enabled = ctx->get_bool(track_changed_notification_enabled_opt, true);
    _settings.is_circled_notification_image = ctx->get_bool(is_circled_notification_image_opt, false);
    
    // playback settings
    _settings.playback_backend_enabled = ctx->get_bool(playback_backend_enabled_opt, true);
    _settings.volume_normalisation_enabled = ctx->get_bool(volume_normalisation_enabled_opt, true);
    _settings.playback_autoplay_enabled = ctx->get_bool(playback_autoplay_enabled_opt, false);
    _settings.gapless_playback_enabled = ctx->get_bool(gapless_playback_enabled_opt, true);
    _settings.playback_cache_enabled = ctx->get_bool(playback_cache_enabled_opt, true);
    // TODO: validate incorrect data from the file
    _settings.playback_bitrate = ctx->get_str(playback_bitrate_opt, playback::bitrate::bps160);
    _settings.playback_format = ctx->get_str(playback_format_opt, playback::format::S16);
    _settings.playback_dither = ctx->get_str(playback_dither_opt, playback::dither::tpdf);
    _settings.playback_volume_ctrl= ctx->get_str(playback_volume_ctrl_opt, playback::volume_ctrl::log);
    _settings.playback_initial_volume = ctx->get_int(playback_initial_volume_opt, 50);

    // views settings
    auto views_settings_json = ctx->get_str(views_settings_opt, "");
    if (!views_settings_json.empty())
        json::parse_to(views_settings_json, _settings.views);

    // hotkeys
    _settings.is_global_hotkeys_enabled = ctx->get_bool(activate_global_hotkeys_opt, true);
    for (int hotkey_id = hotkeys::play; hotkey_id != hotkeys::last; hotkey_id++)
    {
        auto key_and_mods = ctx->get_int64(get_hotkey_node_name(hotkey_id), 0);
        // packing key code and modifiers into one big int
        _settings.hotkeys[hotkey_id] = std::make_pair(HIWORD(key_and_mods), LOWORD(key_and_mods));
    }

    // complimentary config data
    _settings.plugin_startup_folder = get_plugin_launch_folder(info);
    _settings.plugin_data_folder = get_user_app_data_folder();
}

void write()
{
    auto ctx = lock_settings();

    // general
    ctx->set_bool(add_to_disk_menu_opt, _settings.add_to_disk_menu);
    ctx->set_bool(verbose_logging_enabled_opt, _settings.verbose_logging);

    // spotify
    ctx->set_wstr(spotify_client_id_opt, _settings.spotify_client_id);
    ctx->set_wstr(spotify_client_secret_opt, _settings.spotify_client_secret);
    ctx->set_int(localhost_service_port_opt, _settings.localhost_service_port);

    // notifications
    ctx->set_bool(track_changed_notification_enabled_opt, _settings.track_changed_notification_enabled);
    ctx->set_bool(is_circled_notification_image_opt, _settings.is_circled_notification_image);
    
    // playback settings
    ctx->set_bool(playback_backend_enabled_opt, _settings.playback_backend_enabled);
    ctx->set_bool(volume_normalisation_enabled_opt, _settings.volume_normalisation_enabled);
    ctx->set_bool(playback_autoplay_enabled_opt, _settings.playback_autoplay_enabled);
    ctx->set_bool(gapless_playback_enabled_opt, _settings.gapless_playback_enabled);
    ctx->set_bool(playback_cache_enabled_opt, _settings.playback_cache_enabled);
    ctx->set_str(playback_bitrate_opt, _settings.playback_bitrate);
    ctx->set_str(playback_format_opt, _settings.playback_format);
    ctx->set_str(playback_dither_opt, _settings.playback_dither);
    ctx->set_str(playback_volume_ctrl_opt, _settings.playback_volume_ctrl);
    ctx->set_int(playback_initial_volume_opt, _settings.playback_initial_volume);
    
    // view
    auto buffer = json::dump(_settings.views);
    ctx->set_str(views_settings_opt, buffer->GetString());

    // global hotkeys
    ctx->set_bool(activate_global_hotkeys_opt, _settings.is_global_hotkeys_enabled);
    for (const auto &[key, p]: _settings.hotkeys)
        ctx->set_int64(get_hotkey_node_name(key), MAKELONG(p.second, p.first));
}

bool is_added_to_disk_menu()
{
    return _settings.add_to_disk_menu;
}

bool is_global_hotkeys_enabled()
{
    return _settings.is_global_hotkeys_enabled;
}

bool is_verbose_logging_enabled()
{
    return _settings.verbose_logging;
}

string get_client_id()
{
    return utils::to_string(_settings.spotify_client_id);
}

string get_client_secret()
{
    return utils::to_string(_settings.spotify_client_secret);
}

int get_localhost_port()
{
    return _settings.localhost_service_port;
}

const wstring& get_plugin_launch_folder()
{
    return _settings.plugin_startup_folder;
}

const wstring& get_plugin_data_folder()
{
    return _settings.plugin_data_folder;
}

const std::pair<WORD, WORD>* get_hotkey(int hotkey_id)
{
    auto it = _settings.hotkeys.find(hotkey_id);
    if (it != _settings.hotkeys.end())
        return &it->second;
    return nullptr;
}

settings::view_t* get_panel_settings(const string &view_uid, const settings::view_t &def)
{
    auto result = _settings.views.emplace(view_uid, def);
    return &result.first->second;
}

bool is_playback_backend_enabled()
{
    return _settings.playback_backend_enabled;
}

bool is_playback_normalisation_enabled()
{
    return _settings.volume_normalisation_enabled;
}

bool is_playback_autoplay_enabled()
{
    return _settings.playback_autoplay_enabled;
}

bool is_gapless_playback_enabled()
{
    return _settings.gapless_playback_enabled;
}

bool is_playback_cache_enabled()
{
    return _settings.playback_cache_enabled;
}

string get_playback_bitrate()
{
    return _settings.playback_bitrate;
}

string get_playback_format()
{
    return _settings.playback_format;
}

string get_playback_dither()
{
    return _settings.playback_dither;
}

string get_playback_volume_ctrl()
{
    return _settings.playback_volume_ctrl;
}

size_t get_playback_initial_volume()
{
    return _settings.playback_initial_volume;
}

bool is_track_changed_notification_enabled()
{
    return _settings.track_changed_notification_enabled;
}

bool is_notification_image_circled()
{
    return _settings.is_circled_notification_image;
}

} // namespace config
} //namespace spotifar
