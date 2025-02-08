#include "config.hpp"
#include "utils.hpp"

namespace spotifar { namespace config {

static const wchar_t
    *add_to_disk_menu_opt = L"AddToDisksMenu",
    *activate_global_hotkeys_opt = L"ActivateGlobalHotkeys",
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

settings_context::settings_context():
    ps(MainGuid, ps_info.SettingsControl),
    settings_copy(_settings)
    {}

settings_context::~settings_context()
{
    if (_settings.is_global_hotkeys_enabled != settings_copy.is_global_hotkeys_enabled)
        ObserverManager::notify(&config_observer::on_global_hotkeys_setting_changed,
            _settings.is_global_hotkeys_enabled);

    for (const auto &[hotkey_id, hotkey]: _settings.hotkeys)
    {
        auto &old_hotkey = settings_copy.hotkeys[hotkey_id];
        if (hotkey.first != old_hotkey.first || hotkey.second != old_hotkey.second)
            ObserverManager::notify(&config_observer::on_global_hotkey_changed,
                                    hotkey_id, hotkey.first, hotkey.second);
    }
}

settings& settings_context::get_settings()
{
    return _settings;
}

bool settings_context::get_bool(const wstring &name, bool def)
{
    return ps.Get(0, name.c_str(), def);
}

std::int64_t settings_context::get_int64(const wstring &name, std::int64_t def)
{
    return ps.Get(0, name.c_str(), def);
}

int settings_context::get_int(const wstring &name, int def)
{
    return ps.Get(0, name.c_str(), def);
}

const wstring settings_context::get_wstr(const wstring &name, const wstring &def)
{
    return ps.Get(0, name.c_str(), def.c_str());
}

string settings_context::get_str(const wstring &name, const string &def)
{
    return utils::to_string(get_wstr(name, utils::to_wstring(def)));
}

void settings_context::set_bool(const wstring &name, bool value)
{
    ps.Set(0, name.c_str(), value);
}

void settings_context::set_int64(const wstring &name, std::int64_t value)
{
    ps.Set(0, name.c_str(), value);
}

void settings_context::set_int(const wstring &name, int value)
{
    ps.Set(0, name.c_str(), value);
}

void settings_context::set_wstr(const wstring &name, const wstring &value)
{
    ps.Set(0, name.c_str(), value.c_str());
}

void settings_context::set_str(const wstring &name, const string &value)
{
    set_wstr(name, utils::to_wstring(value));
}

bool settings_context::delete_value(const wstring& name)
{
    return ps.DeleteValue(0, name.c_str());
}

std::shared_ptr<settings_context> lock_settings()
{
    return std::make_shared<settings_context>();
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
    
    _settings.add_to_disk_menu = ctx->get_bool(add_to_disk_menu_opt, true);
    _settings.is_global_hotkeys_enabled = ctx->get_bool(activate_global_hotkeys_opt, true);
    _settings.spotify_client_id = ctx->get_wstr(spotify_client_id_opt, L"");
    _settings.spotify_client_secret = ctx->get_wstr(spotify_client_secret_opt, L"");
    _settings.localhost_service_port = ctx->get_int(localhost_service_port_opt, 5050);
    _settings.plugin_startup_folder = get_plugin_launch_folder(info);

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
    ctx->set_wstr(spotify_client_id_opt, _settings.spotify_client_id);
    ctx->set_wstr(spotify_client_secret_opt, _settings.spotify_client_secret);
    ctx->set_int(localhost_service_port_opt, _settings.localhost_service_port);

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

std::pair<WORD, WORD>* get_hotkey(int hotkey_id)
{
    auto it = _settings.hotkeys.find(hotkey_id);
    if (it != _settings.hotkeys.end())
        return &it->second;
    return nullptr;
}

} // namespace config
} //namespace spotifar
