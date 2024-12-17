#include "stdafx.h"
#include "version.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "lng.hpp"
#include "plugin.h"
#include "ui/config_dialog.hpp"

#include <plugin.hpp>

namespace spotifar
{
	void WINAPI GetGlobalInfoW(struct GlobalInfo* info)
	{
		info->StructSize = sizeof(struct GlobalInfo);
		info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 4400, VS_RELEASE);
		info->Version = PLUGIN_VERSION;
		info->Guid = MainGuid;
		info->Title = PLUGIN_NAME;
		info->Description = PLUGIN_DESC;
		info->Author = PLUGIN_AUTHOR;
	}

	void WINAPI SetStartupInfoW(const struct PluginStartupInfo* info)
	{
		config::Opt.read(info);
	}

	void WINAPI GetPluginInfoW(struct PluginInfo* info)
	{
		info->StructSize = sizeof(*info);
		info->Flags = 0;

		if (config::Opt.AddToDisksMenu)
		{
			static const wchar_t* DiskMenuStrings[1];
			DiskMenuStrings[0] = config::get_msg(MPluginUserName);
			info->DiskMenu.Guids = &MenuGuid;
			info->DiskMenu.Strings = DiskMenuStrings;
			info->DiskMenu.Count = std::size(DiskMenuStrings);
		}

		if (TRUE)  // add to plugins menu
		{
			static const wchar_t* PluginMenuStrings[1];
			PluginMenuStrings[0] = config::get_msg(MPluginUserName);
			info->PluginMenu.Guids = &MenuGuid;
			info->PluginMenu.Strings = PluginMenuStrings;
			info->PluginMenu.Count = std::size(PluginMenuStrings);
		}

		// add to plugins configuration menu
		static const wchar_t* PluginCfgStrings[1];
		PluginCfgStrings[0] = config::get_msg(MPluginUserName);
		info->PluginConfig.Guids = &MenuGuid;
		info->PluginConfig.Strings = PluginCfgStrings;
		info->PluginConfig.Count = std::size(PluginCfgStrings);
	}

	HANDLE WINAPI OpenW(const struct OpenInfo* info)
	{
		try 
		{
			// note: logger uses config data, which is being initialized above
			utils::init_logging();
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			auto err_msg = utils::utf8_decode(ex.what());
			utils::show_far_error_dlg(MFarMessageErrorLogInit, err_msg);

			return nullptr;
		}

		return std::make_unique<Plugin>().release();
	}

	void WINAPI GetOpenPanelInfoW(OpenPanelInfo* info)
	{
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		plugin.update_panel_info(info);
	}

	intptr_t WINAPI GetFindDataW(GetFindDataInfo* info)
	{
		if (info->OpMode & OPM_FIND)
			return FALSE;
		
		return static_cast<Plugin*>(info->hPanel)->update_panel_items(info);
	}

	void WINAPI FreeFindDataW(const FreeFindDataInfo* info)
	{
		return static_cast<Plugin*>(info->hPanel)->free_panel_items(info);
	}

	intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo* info)
	{
		if (info->OpMode & OPM_FIND)
			return FALSE;
			
		return static_cast<Plugin*>(info->hPanel)->select_item(info);
	}

	intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo* info)
	{
		return TRUE;
	}

	// https://api.farmanager.com/ru/exported_functions/processpanelinputw.html
	intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo* info)
	{
		auto& plugin = *static_cast<Plugin*>(info->hPanel);

		auto& key_event = info->Rec.Event.KeyEvent;
		if (key_event.bKeyDown)
		{
			int key = utils::input_record_to_combined_key(key_event);
			switch (key)
			{
				case VK_F4:
				{
					return plugin.show_player();
				}
			}
		}

		return FALSE;
	}

	intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo* info)
	{
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		
		if (info->Event == FE_CLOSE)
		{
			static_cast<Plugin*>(info->hPanel)->shutdown();
		}

		return FALSE;
	}

	intptr_t WINAPI ConfigureW(const ConfigureInfo* info)
	{
		return ui::ConfigDialog().show();
	}

	void WINAPI ClosePanelW(const ClosePanelInfo* info)
	{
		// after auto-variable is destroyed, so is the last ref to plugin
		std::unique_ptr<Plugin>(static_cast<Plugin*>(info->hPanel));

		config::Opt.write();
		utils::fini_logging();
	}

	void WINAPI ExitFARW(const struct ExitInfo *Info)
	{
	}
}
