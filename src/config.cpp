#include "config.hpp"
#include "utils.hpp"

namespace spotifar
{
	namespace config
	{
		static const wchar_t *ADD_TO_DISK_MENU_OPT = L"AddToDisksMenu";
		static const wchar_t *SPOTIFY_CLIENT_ID_OPT = L"SpotifyClientID";
		static const wchar_t *SPOTIFY_CLIENT_SECRET_OPT = L"SpotifyClientSecret";
		static const wchar_t *LOCALHOST_SERVICE_PORT_OPT = L"LocalhostServicePort";

		PluginStartupInfo PsInfo;
		FarStandardFunctions FSF;
		Settings settings;

		
		SettingsContext::SettingsContext():
			ps(MainGuid, PsInfo.SettingsControl)
		{}
		
		Settings& SettingsContext::get_settings()
		{
			return settings;
		}
		
		bool SettingsContext::get_bool(const wstring &name, bool def)
		{
			return ps.Get(0, name.c_str(), def);
		}

		std::int64_t SettingsContext::get_int64(const wstring &name, std::int64_t def)
		{
			return ps.Get(0, name.c_str(), def);
		}

		int SettingsContext::get_int(const wstring &name, int def)
		{
			return ps.Get(0, name.c_str(), def);
		}

		const wstring SettingsContext::get_wstr(const wstring &name, const wstring &def)
		{
			return ps.Get(0, name.c_str(), def.c_str());
		}

		string SettingsContext::get_str(const wstring &name, const string &def)
		{
			return utils::to_string(get_wstr(name, utils::to_wstring(def)));
		}

		void SettingsContext::set_bool(const wstring &name, bool value)
		{
			ps.Set(0, name.c_str(), value);
		}

		void SettingsContext::set_int64(const wstring &name, std::int64_t value)
		{
			ps.Set(0, name.c_str(), value);
		}
		
		void SettingsContext::set_int(const wstring &name, int value)
		{
			ps.Set(0, name.c_str(), value);
		}
		
		void SettingsContext::set_wstr(const wstring &name, const wstring &value)
		{
			ps.Set(0, name.c_str(), value.c_str());
		}
		
		void SettingsContext::set_str(const wstring &name, const string &value)
		{
			set_wstr(name, utils::to_wstring(value));
		}
		
		bool SettingsContext::delete_value(const wstring& name)
		{
			return ps.DeleteValue(0, name.c_str());
		}
		
		std::shared_ptr<SettingsContext> lock_settings()
		{
			return std::make_shared<SettingsContext>();
		}

		void read(const PluginStartupInfo *info)
		{
			PsInfo = *info;
			FSF = *info->FSF;
			PsInfo.FSF = &FSF;

			auto ctx = lock_settings();
			
			settings.add_to_disk_menu = ctx->get_bool(ADD_TO_DISK_MENU_OPT, true);
			settings.spotify_client_id = ctx->get_wstr(SPOTIFY_CLIENT_ID_OPT, L"");
			settings.spotify_client_secret = ctx->get_wstr(SPOTIFY_CLIENT_SECRET_OPT, L"");
			settings.localhost_service_port = ctx->get_int(LOCALHOST_SERVICE_PORT_OPT, 5050);
			settings.plugin_startup_folder = utils::far3::get_plugin_launch_folder(info);
		}

		void write()
		{
			auto ctx = lock_settings();

			ctx->set_bool(ADD_TO_DISK_MENU_OPT, settings.add_to_disk_menu);
			ctx->set_wstr(SPOTIFY_CLIENT_ID_OPT, settings.spotify_client_id);
			ctx->set_wstr(SPOTIFY_CLIENT_SECRET_OPT, settings.spotify_client_secret);
			ctx->set_int(LOCALHOST_SERVICE_PORT_OPT, settings.localhost_service_port);
		}

		bool is_added_to_disk_menu()
		{
			return settings.add_to_disk_menu;
		}

		string get_client_id()
		{
			return utils::to_string(settings.spotify_client_id);
		}

		string get_client_secret()
		{
			return utils::to_string(settings.spotify_client_secret);
		}

		int get_localhost_port()
		{
			return settings.localhost_service_port;
		}
		
		const wstring& get_plugin_launch_folder()
		{
			return settings.plugin_startup_folder;
		}
	}
}
