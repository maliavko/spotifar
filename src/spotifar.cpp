#include "stdafx.h"
#include "version.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "browser.hpp"

#include <plugin.hpp>

namespace spotifar
{
	PluginStartupInfo PsInfo;
	FarStandardFunctions FSF;

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
		PsInfo = *info;
		FSF = *info->FSF;
		PsInfo.FSF = &FSF;

		config::Opt.Read(info);
	}

	void WINAPI GetPluginInfoW(struct PluginInfo* info)
	{
		info->StructSize = sizeof(*info);
		info->Flags = 0;

		if (config::Opt.AddToDisksMenu)
		{
			static const wchar_t* DiskMenuStrings[1];
			DiskMenuStrings[0] = L"Spotifar";
			info->DiskMenu.Guids = &MenuGuid;
			info->DiskMenu.Strings = DiskMenuStrings;
			info->DiskMenu.Count = ARRAYSIZE(DiskMenuStrings);
		}

		if (TRUE)
		{
			static const wchar_t* PluginMenuStrings[1];
			PluginMenuStrings[0] = L"Spotifar";
			info->PluginMenu.Guids = &MenuGuid;
			info->PluginMenu.Strings = PluginMenuStrings;
			info->PluginMenu.Count = ARRAYSIZE(PluginMenuStrings);
		}

		static const wchar_t* PluginCfgStrings[1];
		PluginCfgStrings[0] = L"Spotifar";
		info->PluginConfig.Guids = &MenuGuid;
		info->PluginConfig.Strings = PluginCfgStrings;
		info->PluginConfig.Count = ARRAYSIZE(PluginCfgStrings);
	}

	HANDLE WINAPI OpenW(const struct OpenInfo* info)
	{
		auto hPlugin = std::make_unique<Browser>();
		return hPlugin.release();
	}

	void WINAPI ClosePanelW(const ClosePanelInfo* info)
	{
		std::unique_ptr<Browser>(static_cast<Browser*>(info->hPanel));
	}

	void WINAPI GetOpenPanelInfoW(OpenPanelInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);

		info->StructSize = sizeof(*info);
		info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;

		info->CurDir = L"";

		static wchar_t Title[100] = L"TestPanel";
		info->PanelTitle = Title;
	}

	intptr_t WINAPI GetFindDataW(GetFindDataInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);

		if (info->OpMode & OPM_FIND)
			return FALSE;

		PluginPanelItem* NewPanelItem = new PluginPanelItem[1];

		if (NewPanelItem)
		{
			memset(NewPanelItem, 0, sizeof(PluginPanelItem) * 1);

			NewPanelItem[0].FileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;

			NewPanelItem[0].FileName = new wchar_t[lstrlen(L"Playlists") + 1];
			lstrcpy((LPWSTR)NewPanelItem[0].FileName, L"Playlists");

			//NewPanelItem[0].Description = new wchar_t[lstrlen(L"D:\\") + 1];
			//lstrcpy((LPWSTR)NewPanelItem[0].Description, L"D:\\");

			info->PanelItem = NewPanelItem;
			info->ItemsNumber = 1;
			return TRUE;
		}

		info->PanelItem = nullptr;
		info->ItemsNumber = 0;
		return FALSE;
	}

	void WINAPI FreeFindDataW(const FreeFindDataInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);
	}

	intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);

		if (info->OpMode & OPM_FIND)
			return TRUE;

		if (!lstrcmp(info->Dir, L"Playlists"))
		{
			//PsInfo.PanelControl(sdInfo->hPanel, FCTL_CLOSEPANEL, 0, (void*)L"D:\\");
		}

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
		return TRUE;
	}

	intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo* info)
	{
		auto& browser = *static_cast<Browser*>(info->hPanel);
		return TRUE;
	}

	intptr_t WINAPI ConfigureW(const ConfigureInfo* info)
	{
		return config::init();
	}


	void WINAPI ExitFARW(const ExitInfo* eInfo)
	{
	}
}
