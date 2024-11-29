#include "stdafx.h"
#include "version.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "browser.hpp"
#include "lng.hpp"

#include <plugin.hpp>
#include <farcolor.hpp>
#include <algorithm>

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

	HANDLE hdlg1;
	std::wstring track_bar;
	std::wstring artist_name(L"Marilyn Manson");
	std::wstring track_name(L"One Assassination Under God");
	std::wstring source_target(L"Album: ");
	std::wstring source_name(L"Album: One Assassination Under God - Chapter 1"); // 46 symbols
	
	enum
	{
		ID_BOX = 0,
		ID_TRACK_BAR,
		ID_TRACK_TIME,
		ID_TRACK_LENGTH,
		ID_ARTIST_NAME,
		ID_TRACK_NAME,
		ID_SOURCE_NAME,
		ID_PLAY_BTN,
		ID_PREV_BTN,
		ID_NEXT_BTN,
		ID_LIKE_BTN,
		ID_VOLUME_VALUE,
		ID_DEVICES_LIST,
	} DialogControls;

	enum
	{
		CLR_BLACK = 0,
		CLR_BLUE,
		CLR_GREEN,
		CLR_CYAN,
		CLR_RED,
		CLR_PURPLE,
		CLR_BROWN,
		CLR_GRAY,
		CLR_DGRAY,
		CLR_LBLUE,
		CLR_LGREEN,
		CLR_LCYAN,
		CLR_LRED,
		CLR_LPURPLE,
		CLR_YELLOW,
		CLR_WHITE
	} Colors16;

	intptr_t Dlg_OnCtlColorDlgItem(HANDLE hdlg, intptr_t id, void* par2)
	{
		WORD loattr = 0, hiattr = 0;
		intptr_t res;
		FarDialogItemColors* fdic = (FarDialogItemColors*)par2;
		res = 0;

		switch (id)
		{
			case ID_PLAY_BTN:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->BackgroundColor = CLR_DGRAY;
				fdic->Colors->ForegroundColor = CLR_GRAY;
				res = TRUE;
				break;
			case ID_ARTIST_NAME:
				fdic->Flags = FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_PREV_BTN:
				fdic->Flags = FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_NEXT_BTN:
				fdic->Flags = FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_LIKE_BTN:
				fdic->Flags = FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_VOLUME_VALUE:
				fdic->Flags = FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_TRACK_NAME:
				break;
			default:
				res = FALSE;
				break;
		}

		return res;
	}
	
	intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2)
	{
		intptr_t id = param1;
		intptr_t res = 0;

		switch (msg)
		{
			case DN_INITDIALOG:
				break;
			case DN_CONTROLINPUT:
				break;
			case DN_BTNCLICK:
				break;
			case DN_CTLCOLORDLGITEM:
				res = Dlg_OnCtlColorDlgItem(hdlg, param1, param2);
				break;
		default:
			res = config::PsInfo.DefDlgProc(hdlg, msg, param1, param2);
		}
		return res;
	}

	HANDLE WINAPI OpenW(const struct OpenInfo* info)
	{
		int width = 60, height = 10, content_width = width - 2, content_height = height - 2;
		int track_lengh = 328, track_progress = 228;

		// updating a track progress bar status
		track_bar = std::wstring(content_width - 14, L'░');
		float progress_percent = (float)track_progress/track_lengh;
		int progress_whole_chars_length = (int)(track_bar.size() * progress_percent); 
		fill(track_bar.begin(), track_bar.begin() + progress_whole_chars_length, L'█');


		FarListItem items[3] = {
			{ 0, L"1", 0, 0},
			{ 0, L"2", 0, 0},
		};
		FarList* devices = new FarList{sizeof(FarListItem), 2, items};


		const FarDialogItem dlg_items[] = {
   			{ DI_DOUBLEBOX,   0,0, width, height, DIF_SHOWAMPERSAND, L"Spotifar" },
			{ DI_TEXT,    2,content_height-2, content_width, 1, 0, nullptr,nullptr, DIF_CENTERTEXT, track_bar.c_str() },
			{ DI_TEXT,    2,content_height-2, 6, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, L"03:28" },
			{ DI_TEXT,    content_width-5,content_height-2, 6, 1, 0, nullptr,nullptr, DIF_RIGHTTEXT, L"05:28" },
			{ DI_TEXT,    2,content_height-4, content_width, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, artist_name.c_str() },
			{ DI_TEXT,    2,content_height-5, content_width, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, track_name.c_str() },
			{ DI_TEXT,    6, 0, 53, 1, 0, nullptr,nullptr, DIF_CENTERTEXT, source_name.c_str() },
			{ DI_BUTTON,    28, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS, L"[ > ]" },
			{ DI_BUTTON,    24, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS, L"<<" },
			{ DI_BUTTON,    35, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS, L">>" },
			{ DI_BUTTON,    2, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS, L"[+]" },
			{ DI_TEXT,    content_width-6, content_height, 1, 1, 0, nullptr,nullptr, DIF_CENTERTEXT, L"[100%]" },
			{ DI_LISTBOX,    content_width-15, 2, 1, 1, 0, nullptr,nullptr, DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND, L"11" },
		};

		const auto flags = FDLG_SMALLDIALOG | FDLG_NONMODAL;
		intptr_t X1 = -1, Y1 = -1, X2 = width, Y2 = height;
		
		hdlg1 = config::PsInfo.DialogInit(&MainGuid, &PlayerDialogGuid, X1,Y1, X2,Y2, L"Player", dlg_items, std::size(dlg_items), 0, flags, &dlg_proc, NULL);

		assert(hdlg1 != INVALID_HANDLE_VALUE);
		auto r = config::PsInfo.DialogRun(hdlg1);
		config::PsInfo.DialogFree(hdlg1);


		config::PsInfo.SendDlgMessage(hdlg1, DM_ENABLEREDRAW, FALSE, 0);

		config::PsInfo.SendDlgMessage(hdlg1, DM_LISTADDSTR, ID_DEVICES_LIST, (void*)L"111");
		config::PsInfo.SendDlgMessage(hdlg1, DM_LISTADDSTR, ID_DEVICES_LIST, (void*)L"222");
		config::PsInfo.SendDlgMessage(hdlg1, DM_SHOWITEM, ID_DEVICES_LIST, (void*)(TRUE));

		config::PsInfo.SendDlgMessage(hdlg1, DM_SETTEXTPTR, ID_ARTIST_NAME, (void*)L"Test11");

		config::PsInfo.SendDlgMessage(hdlg1, DM_ENABLEREDRAW, TRUE, 0);


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

	static void WINAPI FreeUserData(void* const UserData, const FarPanelItemFreeInfo* const Info)
	{
		delete static_cast<const ItemFarUserData*>(UserData);
	}
	

	intptr_t WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo* Info)
	{
		// config::PsInfo.AdvControl(&MainGuid, ACTL_SYNCHRO, 0, (void*)0);
		if (Info->Event == SE_COMMONSYNCHRO)
		{
			int param = *(int*)(Info->Param);
		}

		return 0;
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
				NewPanelItem[idx].UserData.Data = new ItemFarUserData(item.id);
				NewPanelItem[idx].UserData.FreeData = FreeUserData;
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

		const ItemFarUserData* data = NULL;
		if (info->UserData.Data != NULL)
			data = static_cast<const ItemFarUserData*>(info->UserData.Data);

		browser.handle_item_selected(data);

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
