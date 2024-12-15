#include "stdafx.h"

#include <PluginSettings.hpp>

#include "config.hpp"
#include "guid.hpp"
#include "lng.hpp"

namespace spotifar
{
	namespace config
	{
		PluginStartupInfo PsInfo;
		FarStandardFunctions FSF;
		Options Opt;

		static const wchar_t* StrAddToDisksMenu = L"AddToDisksMenu";
		static const wchar_t* StrSpotifyClientID = L"SpotifyClientID";
		static const wchar_t* StrSpotifyClientSecret = L"SpotifyClientSecret";
		static const wchar_t* StrSpotifyRefreshToken = L"SpotifyRefreshToken";
		static const wchar_t* StrLocalhostServicePort = L"LocalhostServicePort";

		void Options::read(const struct PluginStartupInfo* info)
		{
			PsInfo = *info;
			FSF = *info->FSF;
			PsInfo.FSF = &FSF;

			PluginSettings s(MainGuid, PsInfo.SettingsControl);

			Opt.AddToDisksMenu = s.Get(0, StrAddToDisksMenu, true);
			lstrcpy(Opt.SpotifyClientID, s.Get(0, StrSpotifyClientID, L""));
			lstrcpy(Opt.SpotifyClientSecret, s.Get(0, StrSpotifyClientSecret, L""));
			lstrcpy(Opt.SpotifyRefreshToken, s.Get(0, StrSpotifyRefreshToken, L""));
			Opt.LocalhostServicePort = s.Get(0, StrLocalhostServicePort, int(5050));

			Opt.PluginStartupFolder = utils::get_plugin_launch_folder(info);
		}

		void Options::write()
		{
			PluginSettings s(MainGuid, PsInfo.SettingsControl);

			s.Set(0, StrAddToDisksMenu, Opt.AddToDisksMenu);
			s.Set(0, StrSpotifyClientID, Opt.SpotifyClientID);
			s.Set(0, StrSpotifyClientSecret, Opt.SpotifyClientSecret);
			s.Set(0, StrSpotifyRefreshToken, Opt.SpotifyRefreshToken);
			s.Set(0, StrLocalhostServicePort, Opt.LocalhostServicePort);
		}

		const wchar_t* get_msg(int msg_id)
		{
			return PsInfo.GetMsg(&MainGuid, msg_id);
		}

		void set_option(wchar_t* opt, const std::string& s)
		{
			lstrcpy(opt, utils::to_wstring(s).c_str());
		}
		
		// TODO: looks like an utils function, consider moving
		intptr_t send_dlg_msg(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2)
		{
			return PsInfo.SendDlgMessage(hdlg, msg, param1, param2);
		}
	}
}
