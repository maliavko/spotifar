#include "stdafx.h"
#include "config.hpp"
#include "plugin.h"
#include "ui/config_dialog.hpp"
#include "spotify/observers.hpp"

namespace spotifar
{
	using utils::far3::get_msg;

	void WINAPI GetGlobalInfoW(GlobalInfo *info)
	{
		info->StructSize = sizeof(GlobalInfo);
		info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 4400, VS_RELEASE);
		info->Version = PLUGIN_VERSION;
		info->Guid = MainGuid;
		info->Title = PLUGIN_NAME;
		info->Description = PLUGIN_DESC;
		info->Author = PLUGIN_AUTHOR;
	}

	void WINAPI GetPluginInfoW(PluginInfo *info)
	{
		info->StructSize = sizeof(*info);
		info->Flags = 0;

		if (config::is_added_to_disk_menu())
		{
			static const wchar_t *DiskMenuStrings[1];
			DiskMenuStrings[0] = get_msg(MPluginUserName);
			info->DiskMenu.Guids = &MenuGuid;
			info->DiskMenu.Strings = DiskMenuStrings;
			info->DiskMenu.Count = std::size(DiskMenuStrings);
		}

		if (TRUE) // add to plugins menu
		{
			static const wchar_t *PluginMenuStrings[1];
			PluginMenuStrings[0] = get_msg(MPluginUserName);
			info->PluginMenu.Guids = &MenuGuid;
			info->PluginMenu.Strings = PluginMenuStrings;
			info->PluginMenu.Count = std::size(PluginMenuStrings);
		}

		// add to plugins configuration menu
		static const wchar_t *PluginCfgStrings[1];
		PluginCfgStrings[0] = get_msg(MPluginUserName);
		info->PluginConfig.Guids = &MenuGuid;
		info->PluginConfig.Strings = PluginCfgStrings;
		info->PluginConfig.Count = std::size(PluginCfgStrings);
	}

	void WINAPI SetStartupInfoW(const PluginStartupInfo *info)
	{
		config::read(info);
	}

	HANDLE WINAPI OpenW(const OpenInfo *info)
	{
		try 
		{
			// note: logger uses config data, which is being initialized before
			utils::init_logging();
		}
		catch (const spdlog::spdlog_ex &ex)
		{
			utils::far3::show_far_error_dlg(
				MFarMessageErrorLogInit, utils::utf8_decode(ex.what()));

			return nullptr;
		}

		return std::make_unique<Plugin>().release();
	}

	void WINAPI GetOpenPanelInfoW(OpenPanelInfo *info)
	{
		// https://api.farmanager.com/ru/structures/openpanelinfo.html
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		plugin.update_panel_info(info);
	}

	intptr_t WINAPI GetFindDataW(GetFindDataInfo *info)
	{
		// https://api.farmanager.com/ru/structures/getfinddatainfo.html
		if (info->OpMode & OPM_FIND)
			return FALSE;
		
		return static_cast<Plugin*>(info->hPanel)->update_panel_items(info);
	}

	void WINAPI FreeFindDataW(const FreeFindDataInfo *info)
	{
		return static_cast<Plugin*>(info->hPanel)->free_panel_items(info);
	}

	intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo *info)
	{
		if (info->OpMode & OPM_FIND)
			return FALSE;
			
		return static_cast<Plugin*>(info->hPanel)->select_item(info);
	}

	// https://api.farmanager.com/ru/exported_functions/processpanelinputw.html
	intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo *info)
	{
		auto& plugin = *static_cast<Plugin*>(info->hPanel);

		auto& key_event = info->Rec.Event.KeyEvent;
		if (key_event.bKeyDown)
		{
			int key = utils::far3::input_record_to_combined_key(key_event);
			switch (key)
			{
				case VK_F4:
				{
					return plugin.show_player();
				}
				case VK_F3:
				{
					config::PsInfo.PanelControl(PANEL_PASSIVE, FCTL_SETVIEWMODE, 2, NULL);
					spdlog::debug("F3");
					//config::PsInfo.PanelControl(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
					return TRUE;
				}
			}
		}

		return FALSE;
	}

	intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo *info)
	{
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		
		if (info->Event == FE_CLOSE)
		{
			static_cast<Plugin*>(info->hPanel)->shutdown();
		}

		return FALSE;
	}

	intptr_t WINAPI ConfigureW(const ConfigureInfo *info)
	{
		return ui::ConfigDialog().show();
	}

	void WINAPI ClosePanelW(const ClosePanelInfo* info)
	{
		// after auto-variable is destroyed, so is the last ref to plugin
		std::unique_ptr<Plugin>(static_cast<Plugin*>(info->hPanel));

		config::write();
		utils::fini_logging();
		utils::far3::clear_synchro_events();
	}

	intptr_t WINAPI ProcessSynchroEventW(const ProcessSynchroEventInfo *info)
	{
		if (info->Event == SE_COMMONSYNCHRO)
		{
			utils::far3::process_synchro_event((intptr_t)info->Param);
			return NULL;
		}
		return NULL;
	}

	HANDLE WINAPI AnalyseW(const AnalyseInfo *info)
	{
		spdlog::debug("HANDLE WINAPI AnalyseW(const AnalyseInfo *info)");
		return NULL;
	}

	intptr_t WINAPI GetFilesW(GetFilesInfo *Info)
	{
		spdlog::debug("intptr_t WINAPI GetFilesW(GetFilesInfo *Info)");
		
		auto file = std::format(L"{}\\{}.txt", Info->DestPath, Info->PanelItem[0].FileName);
		std::ofstream fout(file, std::ios::trunc);
		fout << "Test data" << std::endl;
		fout.close();

		return TRUE;
	}

	intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)
	{
		spdlog::debug("intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)");
		return TRUE;
	}

	void WINAPI ExitFARW(const ExitInfo *info)
	{
		// spdlog::debug("void WINAPI ExitFARW(const ExitInfo *info)");
	}
}
