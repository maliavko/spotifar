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
		static const wchar_t* StrLocalhostServicePort = L"LocalhostServicePort";

		int init()
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
				Opt.Write();
				return TRUE;
			}

			return FALSE;
		}

		void Options::Read(const struct PluginStartupInfo* info)
		{
			PsInfo = *info;
			FSF = *info->FSF;
			PsInfo.FSF = &FSF;

			PluginSettings settings(MainGuid, PsInfo.SettingsControl);

			Opt.AddToDisksMenu = settings.Get(0, StrAddToDisksMenu, true);
			lstrcpy(Opt.SpotifyClientID, settings.Get(0, StrSpotifyClientID, L""));
			lstrcpy(Opt.SpotifyClientSecret, settings.Get(0, StrSpotifyClientSecret, L""));
			Opt.LocalhostServicePort = settings.Get(0, StrLocalhostServicePort, int(5050));

			Opt.PluginStartupFolder = utils::get_plugin_launch_folder(info);
		}

		void Options::Write()
		{
			PluginSettings settings(MainGuid, PsInfo.SettingsControl);

			settings.Set(0, StrAddToDisksMenu, Opt.AddToDisksMenu);
			settings.Set(0, StrSpotifyClientID, Opt.SpotifyClientID);
			settings.Set(0, StrSpotifyClientSecret, Opt.SpotifyClientSecret);
			settings.Set(0, StrLocalhostServicePort, Opt.LocalhostServicePort);
		}

		const wchar_t* get_msg(int msg_id)
		{
			return PsInfo.GetMsg(&MainGuid, msg_id);
		}
	}
}
