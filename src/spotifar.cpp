#include "stdafx.h"
#include "version.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "browser.hpp"
#include "lng.hpp"

#include <plugin.hpp>

namespace spotifar
{
	void WINAPI GetGlobalInfoW(struct GlobalInfo* info)
	{
		info->StructSize = sizeof(struct GlobalInfo);
		info->MinFarVersion = FARMANAGERVERSION;
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
			DiskMenuStrings[0] = config::get_msg(MDiskMenuLabel);
			info->DiskMenu.Guids = &MenuGuid;
			info->DiskMenu.Strings = DiskMenuStrings;
			info->DiskMenu.Count = std::size(DiskMenuStrings);
		}

		if (TRUE)
		{
			static const wchar_t* PluginMenuStrings[1];
			PluginMenuStrings[0] = config::get_msg(MPluginMenuLabel);
			info->PluginMenu.Guids = &MenuGuid;
			info->PluginMenu.Strings = PluginMenuStrings;
			info->PluginMenu.Count = std::size(PluginMenuStrings);
		}

		static const wchar_t* PluginCfgStrings[1];
		PluginCfgStrings[0] = config::get_msg(MConfigMenuLabel);
		info->PluginConfig.Guids = &MenuGuid;
		info->PluginConfig.Strings = PluginCfgStrings;
		info->PluginConfig.Count = std::size(PluginCfgStrings);
	}

	HANDLE WINAPI OpenW(const struct OpenInfo* info)
	{
		auto hPlugin = std::make_unique<Browser>();
		return hPlugin.release();
	}

	void WINAPI ClosePanelW(const ClosePanelInfo* info)
	{
		// wrapping pointer with unique_ptr, to be disposed properly
		std::unique_ptr<Browser>(static_cast<Browser*>(info->hPanel));
		
		config::Opt.write();
	}

	void WINAPI GetOpenPanelInfoW(OpenPanelInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);

		info->StructSize = sizeof(*info);
		info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;
		info->CurDir = _wcsdup(browser.get_target_name().c_str());

		static wchar_t Title[MAX_PATH];
		config::FSF.sprintf(Title, L" %s: %s", config::get_msg(MPanelTitle), info->CurDir);
		info->PanelTitle = Title;
	}

	intptr_t WINAPI GetFindDataW(GetFindDataInfo* info)
	{
		if (info->OpMode & OPM_FIND)
			return FALSE;

		auto& browser = *static_cast<Browser*>(info->hPanel);
		auto items = browser.get_items();
	
		auto* NewPanelItem = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items.size());
		if (NewPanelItem)
		{
			for (size_t idx = 0; idx < items.size(); idx++)
			{
				auto& item = items[idx];
				
				memset(&NewPanelItem[idx], 0, sizeof(PluginPanelItem));
				NewPanelItem[idx].FileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
				NewPanelItem[idx].FileName = _wcsdup(item.name.c_str());
				NewPanelItem[idx].Description = _wcsdup(item.description.c_str());
			}

			info->PanelItem = NewPanelItem;
			info->ItemsNumber = items.size();

			return TRUE;
		}

		info->PanelItem = nullptr;
		info->ItemsNumber = 0;
		return FALSE;
	}

	void WINAPI FreeFindDataW(const FreeFindDataInfo* info)
	{
		for (size_t idx = 0; idx < info->ItemsNumber; idx++)
		{
			free(const_cast<wchar_t*>(info->PanelItem[idx].FileName));
			free(const_cast<wchar_t*>(info->PanelItem[idx].Description));
		}

		free(info->PanelItem);
	}

	intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo* info)
	{
		if (info->OpMode & OPM_FIND)
			return FALSE;

		auto& browser = *static_cast<Browser*>(info->hPanel);
		browser.handle_item_selected(info->Dir);

		return TRUE;
	}

	intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);
		return TRUE;
	}

	intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);

		// https://api.farmanager.com/ru/exported_functions/processpanelinputw.html

		return FALSE;
	}

	intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);
		
		if (info->Event == FE_CLOSE)
		{
			// panel is closing, a right time to save settings and so on
		}
		// the rest: https://api.farmanager.com/ru/structures/processpaneleventinfo.html

		return FALSE;
	}

	intptr_t WINAPI ConfigureW(const ConfigureInfo* info)
	{
		return config::show_dialog();
	}

	void WINAPI ExitFARW(const ExitInfo* eInfo)
	{
	}
}
