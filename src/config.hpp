#ifndef CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#define CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#pragma once

#include <plugin.hpp>
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
			int LocalhostServicePort;
			std::wstring PluginStartupFolder;

			static void Read(const struct PluginStartupInfo* psInfo);
			static void Write();
		} Opt;

		int init();

		const wchar_t* get_msg(int msg_id);
	}
}

#endif //CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
