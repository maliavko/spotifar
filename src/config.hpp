#ifndef CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#define CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#pragma once

#include "stdafx.h"

namespace spotifar
{
	namespace config
	{
		using std::wstring;

		extern PluginStartupInfo PsInfo;
		extern FarStandardFunctions FSF;

		struct Settings
		{
			bool add_to_disk_menu;
			int localhost_service_port;
			std::wstring spotify_client_id, spotify_client_secret;
			std::wstring plugin_startup_folder;
		};

		class SettingsContext
		{
		public:
			SettingsContext();

			bool get_bool(const wstring &name, bool def);
			std::int64_t get_int64(const wstring &name, std::int64_t def);
			int get_int(const wstring &name, int def);
			const wstring get_wstr(const wstring &name, const wstring &def);
			std::string get_str(const wstring &name, const std::string &def);
			
			void set_bool(const wstring &name, bool value);
			void set_int64(const wstring &name, std::int64_t value);
			void set_int(const wstring &name, int value);
			void set_wstr(const wstring &name, const std::wstring &value);
			void set_str(const wstring &name, const std::string &value);

			bool delete_value(const wstring& name);

			Settings& get_settings();
		private:
			PluginSettings ps;
		};

		std::shared_ptr<SettingsContext> lock_settings();

		void read(const PluginStartupInfo *info);
		void write();
		
		bool is_added_to_disk_menu();

		std::string get_client_id();

		std::string get_client_secret();

		int get_localhost_port();

		const std::wstring& get_plugin_launch_folder();
	}
}

#endif //CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B