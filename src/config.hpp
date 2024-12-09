#ifndef CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#define CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#pragma once

#include <plugin.hpp>
#include <PluginSettings.hpp>
#include <string>

namespace spotifar
{
	namespace config
	{
		extern PluginStartupInfo PsInfo;
		extern FarStandardFunctions FSF;

		extern struct Options
		{
			int AddToDisksMenu;
			wchar_t SpotifyClientID[64];
			wchar_t SpotifyClientSecret[64];
			wchar_t SpotifyRefreshToken[256];
			int LocalhostServicePort;

			std::wstring PluginStartupFolder;

			static void read(const struct PluginStartupInfo* info);
			static void write();
		} Opt;

		const wchar_t* get_msg(int msg_id);
		void set_option(wchar_t* opt, const std::string& s);
	}
}

#endif //CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
