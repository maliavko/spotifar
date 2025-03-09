#include "config.hpp"
#include "utils.hpp"

namespace spotifar { namespace config {

using utils::far3::synchro_tasks::dispatch_event;

static const wchar_t
    *add_to_disk_menu_opt = L"AddToDisksMenu",
    *activate_global_hotkeys_opt = L"ActivateGlobalHotkeys",
    *verbose_logging_enabled_opt = L"EnableVerboseLogging",
    *views_settings_opt = L"ViewsSettings",
    *spotify_client_id_opt = L"SpotifyClientID",
    *spotify_client_secret_opt = L"SpotifyClientSecret",
    *localhost_service_port_opt = L"LocalhostServicePort";

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
        dispatch_event(&config_observer::on_global_hotkeys_setting_changed,
            _settings.is_global_hotkeys_enabled);

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
        dispatch_event(&config_observer::on_logging_verbocity_changed,
            _settings.verbose_logging);

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

    _settings.spotify_client_id = ctx->get_wstr(spotify_client_id_opt, L"");
    _settings.spotify_client_secret = ctx->get_wstr(spotify_client_secret_opt, L"");
    _settings.localhost_service_port = ctx->get_int(localhost_service_port_opt, 5050);
    _settings.add_to_disk_menu = ctx->get_bool(add_to_disk_menu_opt, true);
    _settings.is_global_hotkeys_enabled = ctx->get_bool(activate_global_hotkeys_opt, true);
    _settings.verbose_logging = ctx->get_bool(verbose_logging_enabled_opt, false);

    auto views_settings_json = ctx->get_str(views_settings_opt, "");
    if (!views_settings_json.empty())
        json::parse(views_settings_json).get_to(_settings.views);

    _settings.plugin_startup_folder = get_plugin_launch_folder(info);
    _settings.plugin_data_folder = get_user_app_data_folder();

    for (int hotkey_id = hotkeys::play; hotkey_id != hotkeys::last; hotkey_id++)
    {
        auto key_and_mods = ctx->get_int64(get_hotkey_node_name(hotkey_id), 0);
        // packing key code and modifiers into one big int
        _settings.hotkeys[hotkey_id] = std::make_pair(HIWORD(key_and_mods), LOWORD(key_and_mods));
    }
}

void write()
{
    auto ctx = lock_settings();

    ctx->set_bool(add_to_disk_menu_opt, _settings.add_to_disk_menu);
    ctx->set_bool(activate_global_hotkeys_opt, _settings.is_global_hotkeys_enabled);
    ctx->set_bool(verbose_logging_enabled_opt, _settings.verbose_logging);
    ctx->set_wstr(spotify_client_id_opt, _settings.spotify_client_id);
    ctx->set_wstr(spotify_client_secret_opt, _settings.spotify_client_secret);
    ctx->set_int(localhost_service_port_opt, _settings.localhost_service_port);
    ctx->set_str(views_settings_opt, json(_settings.views).dump());

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
    _settings.views.emplace(view_uid, def);
    return &_settings.views.at(view_uid);
}

} // namespace config
} //namespace spotifar
