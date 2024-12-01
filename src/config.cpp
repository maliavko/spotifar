#include "stdafx.h"

#include <DlgBuilder.hpp>
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

		int show_dialog()
		{
			PluginDialogBuilder Builder(spotifar::config::PsInfo, MainGuid, ConfigDialogGuid, MConfigTitle, L"Config");
			Builder.AddCheckbox(MConfigAddToDisksMenu, &Opt.AddToDisksMenu);

			Builder.AddSeparator(MConfigSpotifySettings);
			Builder.AddText(MConfigSpotifyClientID);
			Builder.AddEditField(Opt.SpotifyClientID, ARRAYSIZE(Opt.SpotifyClientID), 40, L"", true);
			Builder.AddText(MConfigSpotifyClientSecret);
			Builder.AddEditField(Opt.SpotifyClientSecret , ARRAYSIZE(Opt.SpotifyClientSecret), 40, L"", true);
			Builder.AddText(MConfigLocalhostServicePort);
			Builder.AddIntEditField(&Opt.LocalhostServicePort, 10);

			Builder.AddOKCancel(MOk, MCancel);

			if (Builder.ShowDialog())
			{
				Opt.write();
				return TRUE;
			}

			return FALSE;
		}

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
		
		std::string to_str(const wchar_t* opt)
		{
			std::wstring s(opt);

			// we do not keep unicode string in the file, so not afraid of the warning
			#pragma warning(suppress: 4244)  
			return std::string(s.begin(), s.end());
		}

		void set_option(wchar_t* opt, const std::string& s)
		{
			lstrcpy(opt, utils::to_wstring(s).c_str());
		}
	}
}
