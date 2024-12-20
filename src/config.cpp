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

		static const wchar_t* add_to_disk_menu_opt = L"AddToDisksMenu";
		static const wchar_t* spotify_client_id_opt = L"SpotifyClientID";
		static const wchar_t* spotify_client_secret_opt = L"SpotifyClientSecret";
		static const wchar_t* spotify_refresh_token_opt = L"SpotifyRefreshToken";
		static const wchar_t* localhost_service_port_opt = L"LocalhostServicePort";
		static const wchar_t* history_opt = L"RecentHistory";
		static const wchar_t* history_timestamp_opt = L"RecentHistoryTimestamp";

		void Options::read(const struct PluginStartupInfo* info)
		{
			PsInfo = *info;
			FSF = *info->FSF;
			PsInfo.FSF = &FSF;

			PluginSettings s(MainGuid, PsInfo.SettingsControl);

			Opt.AddToDisksMenu = s.Get(0, add_to_disk_menu_opt, true);
			lstrcpy(Opt.SpotifyClientID, s.Get(0, spotify_client_id_opt, L""));
			lstrcpy(Opt.SpotifyClientSecret, s.Get(0, spotify_client_secret_opt, L""));
			lstrcpy(Opt.SpotifyRefreshToken, s.Get(0, spotify_refresh_token_opt, L""));
			Opt.LocalhostServicePort = s.Get(0, localhost_service_port_opt, int(5050));
			
			Opt.RecentHistory = utils::to_string(s.Get(0, history_opt, L""));
			Opt.RecentHistoryTimestamp = s.Get(0, history_timestamp_opt, (long long)0);

			Opt.PluginStartupFolder = utils::get_plugin_launch_folder(info);
		}

		void Options::write()
		{
			PluginSettings s(MainGuid, PsInfo.SettingsControl);

			s.Set(0, add_to_disk_menu_opt, Opt.AddToDisksMenu);
			s.Set(0, spotify_client_id_opt, Opt.SpotifyClientID);
			s.Set(0, spotify_client_secret_opt, Opt.SpotifyClientSecret);
			s.Set(0, spotify_refresh_token_opt, Opt.SpotifyRefreshToken);
			s.Set(0, localhost_service_port_opt, Opt.LocalhostServicePort);
			
			s.Set(0, history_opt, utils::to_wstring(Opt.RecentHistory).c_str());
			s.Set(0, history_timestamp_opt, Opt.RecentHistoryTimestamp);
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
