#include "stdafx.h"
#include "version.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "lng.hpp"
#include "plugin.h"

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

		if (TRUE)  // add to plugins menu
		{
			static const wchar_t* PluginMenuStrings[1];
			PluginMenuStrings[0] = config::get_msg(MPluginMenuLabel);
			info->PluginMenu.Guids = &MenuGuid;
			info->PluginMenu.Strings = PluginMenuStrings;
			info->PluginMenu.Count = std::size(PluginMenuStrings);
		}

		// add to plugins configuration menu
		static const wchar_t* PluginCfgStrings[1];
		PluginCfgStrings[0] = config::get_msg(MConfigMenuLabel);
		info->PluginConfig.Guids = &MenuGuid;
		info->PluginConfig.Strings = PluginCfgStrings;
		info->PluginConfig.Count = std::size(PluginCfgStrings);
	}

	int volume = 100;
	HANDLE hdlg1;
	std::wstring track_bar;
	std::wstring artist_name(L"Marilyn Manson");
	std::wstring track_name(L"One Assassination Under God");
	std::wstring source_name(L"One Assassination Under God - Chapter 1"); // 46 symbols
	
	enum
	{
		ID_BOX = 0,
		ID_TRACK_BAR,
		ID_TRACK_TIME,
		ID_TRACK_LENGTH,
		ID_ARTIST_NAME,
		ID_TRACK_NAME,
		//ID_SOURCE_NAME,
		ID_PLAY_BTN,
		ID_PREV_BTN,
		ID_NEXT_BTN,
		ID_LIKE_BTN,
		ID_VOLUME_VALUE,
		ID_DEVICES_COMBO,
		ID_REPEAT_BTN,
		ID_SHUFFLE_BTN,
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
				fdic->Colors->ForegroundColor = CLR_BLACK;
				res = TRUE;
				break;
			case ID_ARTIST_NAME:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_PREV_BTN:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->BackgroundColor = CLR_DGRAY;
				fdic->Colors->ForegroundColor = CLR_BLACK;
				break;
			case ID_NEXT_BTN:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->BackgroundColor = CLR_DGRAY;
				fdic->Colors->ForegroundColor = CLR_BLACK;
				break;
			case ID_LIKE_BTN:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_SHUFFLE_BTN:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			// case ID_SOURCE_NAME:
			// 	fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
			// 	fdic->Colors->ForegroundColor = CLR_DGRAY;
			// 	break;
			case ID_DEVICES_COMBO:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_BLACK;
				fdic->Colors->BackgroundColor = CLR_DGRAY;
				break;
			case ID_VOLUME_VALUE:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				//fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_TRACK_BAR:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = CLR_BLACK;
				break;
			case ID_TRACK_LENGTH:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				//fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_TRACK_TIME:
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				//fdic->Colors->ForegroundColor = CLR_DGRAY;
				break;
			case ID_REPEAT_BTN:
			{
				int color = CLR_DGRAY;
				FarDialogItemData data = { sizeof(FarDialogItemData), 0, new wchar_t[32] };
				auto i = config::PsInfo.SendDlgMessage(hdlg1, DM_GETTEXT, ID_REPEAT_BTN, &data);
				
				if (lstrcmp(data.PtrData, L"Repeat"))
				{
					color = CLR_BLACK;
				}
				fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
				fdic->Colors->ForegroundColor = color;
				break;
			}
			default:
				res = FALSE;
				break;
		}

		return res;
	}

	static const int CTRL = 0x100000, ALT = 0x200000, SHIFT = 0x400000;
	
	intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2)
	{
		intptr_t id = param1;
		intptr_t res = 0;
		INPUT_RECORD* ir;

		switch (msg)
		{
			case DN_INITDIALOG:
				break;
			case DN_BTNCLICK:
				break;
			case DN_CTLCOLORDLGITEM:
				res = Dlg_OnCtlColorDlgItem(hdlg, param1, param2);
				break;
			case DN_CONTROLINPUT:
			{
				ir = (INPUT_RECORD*)param2;
				switch (ir->EventType)
				{
					case KEY_EVENT:
						if (ir->Event.KeyEvent.bKeyDown)
						{
							int key = static_cast<int>(ir->Event.KeyEvent.wVirtualKeyCode);
							const auto state = ir->Event.KeyEvent.dwControlKeyState;
							
							if (state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) key |= CTRL;
							if (state & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) key |= ALT;
							if (state & SHIFT_PRESSED) key |= SHIFT;

							switch (key)
							{
								case VK_UP:
								case VK_DOWN:
								{
									volume += key == VK_UP ? +5 : -5;
									volume = min(max(volume, 0), 100);
									std::wstring volume_label = std::format(L"[{:3}%]", volume);
									config::PsInfo.SendDlgMessage(hdlg1, DM_SETTEXTPTR, ID_VOLUME_VALUE, (void*)volume_label.c_str());
									break;
								}
							}
						}
						break;
					case MOUSE_EVENT:
					{
						if (param1 == ID_REPEAT_BTN)
						{
							FarDialogItemData data = { sizeof(FarDialogItemData), 0, new wchar_t[32] };
							config::PsInfo.SendDlgMessage(hdlg1, DM_GETTEXT, ID_REPEAT_BTN, &data);
							
							if (!lstrcmp(data.PtrData, L"Repeat"))
							{
								config::PsInfo.SendDlgMessage(hdlg1, DM_SETTEXTPTR, ID_REPEAT_BTN, (void*)L"Track");
							}
							if (!lstrcmp(data.PtrData, L"Track"))
							{
								config::PsInfo.SendDlgMessage(hdlg1, DM_SETTEXTPTR, ID_REPEAT_BTN, (void*)L"All");
							}
							if (!lstrcmp(data.PtrData, L"All"))
							{
								config::PsInfo.SendDlgMessage(hdlg1, DM_SETTEXTPTR, ID_REPEAT_BTN, (void*)L"Repeat");
							}
							res = true;
						}
						break;
					}
				}
				break;
			}
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
			{ LIF_SELECTED, L"Librespot", 0, 0},
			{ 0, L"Web Player", 0, 0},
			{ 0, L"Shield TV", 0, 0},
		};
		FarList* devices = new FarList{sizeof(FarListItem), 3, items};


		std::wstring volume_label = std::format(L"[{:3}%]", volume);
		const FarDialogItem dlg_items[] = {
   			{ DI_DOUBLEBOX,   0,0, width, height, DIF_SHOWAMPERSAND, L"Spotifar" },
			{ DI_TEXT,    2,content_height-2, content_width, 1, 0, nullptr,nullptr, DIF_CENTERTEXT, track_bar.c_str() },
			{ DI_TEXT,    2,content_height-2, 6, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, L"03:28" },
			{ DI_TEXT,    content_width-5,content_height-2, 6, 1, 0, nullptr,nullptr, DIF_RIGHTTEXT, L"05:28" },
			{ DI_TEXT,    2,content_height-5, content_width, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, artist_name.c_str() },
			{ DI_TEXT,    2,content_height-4, content_width, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, track_name.c_str() },
			//{ DI_TEXT,    6, 0, 53, 1, 0, nullptr,nullptr, DIF_CENTERTEXT, source_name.c_str() },
			//{ DI_TEXT,    2, content_height-6, content_width, 1, 0, nullptr,nullptr, DIF_LEFTTEXT, source_name.c_str() },
			{ DI_BUTTON,    28, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOFOCUS | DIF_BTNNOCLOSE, L">" },
			{ DI_BUTTON,    23, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, L"[<<]" },
			{ DI_BUTTON,    34, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, L"[>>]" },
			{ DI_BUTTON,    2, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, L"[+]" },
			{ DI_TEXT,    content_width-6, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_CENTERTEXT | DIF_NOFOCUS | DIF_BTNNOCLOSE, volume_label.c_str() },
			{ .Type=DI_COMBOBOX,    .X1=content_width-13, .Y1=1, .X2=content_width-1, .Y2=0, .ListItems=devices, .History=nullptr, .Mask=nullptr, .Flags=DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND | DIF_DROPDOWNLIST | DIF_NOFOCUS, .Data=L"" },
			{ DI_BUTTON,    39, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, L"Repeat" },
			{ DI_BUTTON,    15, content_height, 1, 1, 0, nullptr,nullptr, DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, L"Shuffle" },
		};

		const auto flags = FDLG_SMALLDIALOG | FDLG_NONMODAL;
		intptr_t X1 = -1, Y1 = -1, X2 = width, Y2 = height;
		
		//hdlg1 = config::PsInfo.DialogInit(&MainGuid, &PlayerDialogGuid, X1,Y1, X2,Y2, L"Player", dlg_items, std::size(dlg_items), 0, flags, &dlg_proc, NULL);

		//assert(hdlg1 != INVALID_HANDLE_VALUE);
		// auto r = config::PsInfo.DialogRun(hdlg1);
		// config::PsInfo.DialogFree(hdlg1);

		//config::PsInfo.SendDlgMessage(hdlg1, DM_ENABLEREDRAW, FALSE, 0);
		//config::PsInfo.SendDlgMessage(hdlg1, DM_LISTADDSTR, ID_DEVICES_COMBO, (void*)L"111");
		//config::PsInfo.SendDlgMessage(hdlg1, DM_ENABLEREDRAW, TRUE, 0);
		//config::PsInfo.SendDlgMessage(hdlg1, DM_SETINPUTNOTIFY, TRUE, 0);

		auto hPlugin = std::make_unique<Plugin>();
		return hPlugin.release();
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
		
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		return plugin.update_panel_items(info);
	}

	void WINAPI FreeFindDataW(const FreeFindDataInfo* info)
	{
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		return plugin.free_panel_items(info);
	}

	intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo* info)
	{
		if (info->OpMode & OPM_FIND)
			return FALSE;
			
		auto& plugin = *static_cast<Plugin*>(info->hPanel);
		return plugin.select_item(info);
	}

	intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo* info)
	{
		return TRUE;
	}

	intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo* info)
	{
		// auto& browser = *static_cast<Plugin*>(info->hPanel);

		// // https://api.farmanager.com/ru/exported_functions/processpanelinputw.html

		// auto& key_event = info->Rec.Event.KeyEvent;
		// if (key_event.bKeyDown)
		// {
		// 	int key = static_cast<int>(key_event.wVirtualKeyCode);
		// 	const auto state = key_event.dwControlKeyState;
			
		// 	if (state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) key |= CTRL;
		// 	if (state & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) key |= ALT;
		// 	if (state & SHIFT_PRESSED) key |= SHIFT;

		// 	switch (key)
		// 	{
		// 		case VK_F4:
		// 		{
		// 			browser.get_api().get_player().set_active(true);
		// 			return TRUE;
		// 			break;
		// 		}
		// 	}
		// }

		return FALSE;
	}

	intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo* info)
	{
		auto& browser = *static_cast<Plugin*>(info->hPanel);
		
		if (info->Event == FE_CLOSE)
		{
			// panel is closing, a right time to save settings and so on
			// the rest: https://api.farmanager.com/ru/structures/processpaneleventinfo.html
		}

		return FALSE;
	}

	intptr_t WINAPI ConfigureW(const ConfigureInfo* info)
	{
		// TODO: move dialog to "ui" folder
		return config::show_dialog();
	}

	void WINAPI ClosePanelW(const ClosePanelInfo* info)
	{
		// after auto-variable is destroyed, the last ref to plugin is as well
		std::unique_ptr<Plugin>(static_cast<Plugin*>(info->hPanel));

		config::Opt.write();
	}

	void WINAPI ExitFARW(const ExitInfo* eInfo)
	{
	}
}
