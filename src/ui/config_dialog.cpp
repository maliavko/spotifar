#include "stdafx.h"
#include "config.hpp"
#include "utils.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
    namespace ui
    {
        static const int width = 61, height = 25;
        static const int view_x = 5, view_y = 3, view_width = width - 2, view_height = height - 2;
        static const int view_center_x = (view_width + view_x)/2, view_center_y = (view_height + view_y)/2;

        static FarDialogItem control(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
                                     FARDIALOGITEMFLAGS flags, const wchar_t *data = L"")
        {
            return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
        }
    
        static std::vector<FarDialogItem> dlg_items_layout{
            // border
            control(DI_DOUBLEBOX,   3, 1, width - 4, height - 2,                            DIF_NONE), // BOX
            control(DI_TEXT,        view_center_x - 10, 1, view_center_x + 10, 1,   DIF_CENTERTEXT), // TITLE
            
            control(DI_TEXT,     0, view_y + 1, 60, 1,             DIF_SEPARATOR),

            // api settings
            control(DI_TEXT,        view_x, view_y + 2, view_x + 15, 1,             DIF_LEFTTEXT),
            control(DI_EDIT,     view_x + 15, view_y + 2, view_x + 50, 1,             DIF_LEFTTEXT),
            control(DI_TEXT,        view_x, view_y + 3, view_x + 15, 1,             DIF_LEFTTEXT),
            control(DI_EDIT,     view_x + 15, view_y + 3, view_x + 50, 1,             DIF_LEFTTEXT),
            control(DI_TEXT,        view_x, view_y + 4, view_x + 15, 1,             DIF_LEFTTEXT),
            control(DI_EDIT,     view_x + 15, view_y + 4, view_x + 50, 1,             DIF_LEFTTEXT),
            
            control(DI_TEXT,     0, view_y + 6, 60, 1,             DIF_SEPARATOR),

            control(DI_TEXT,     view_x + 33, view_y + 7, view_x + 38, 1,             DIF_LEFTTEXT),
            control(DI_TEXT,     view_x + 39, view_y + 7, view_x + 45, 1,             DIF_LEFTTEXT),
            control(DI_TEXT,     view_x + 47, view_y + 7, view_x + 50, 1,             DIF_LEFTTEXT),

            control(DI_TEXT,     view_x, view_y + 8, view_x + 15, 1,             DIF_LEFTTEXT),
            control(DI_EDIT,     view_x + 15, view_y + 8, view_x + 29, 1,             DIF_LEFTTEXT),
            control(DI_CHECKBOX,     view_x + 33, view_y + 8, view_x, 1,             DIF_LEFTTEXT),
            control(DI_CHECKBOX,     view_x + 40, view_y + 8, view_x, 1,             DIF_LEFTTEXT),
            control(DI_CHECKBOX,     view_x + 47, view_y + 8, view_x, 1,             DIF_LEFTTEXT),

            control(DI_TEXT,     view_x, view_y + 9, view_x + 15, 1,             DIF_LEFTTEXT),
            control(DI_EDIT,     view_x + 15, view_y + 9, view_x + 29, 1,             DIF_LEFTTEXT),
            control(DI_CHECKBOX,     view_x + 33, view_y + 9, view_x, 1,             DIF_LEFTTEXT),
            control(DI_CHECKBOX,     view_x + 40, view_y + 9, view_x, 1,             DIF_LEFTTEXT),
            control(DI_CHECKBOX,     view_x + 47, view_y + 9, view_x, 1,             DIF_LEFTTEXT),
            
            control(DI_CHECKBOX,     view_x, view_y + 6, view_x, 1,             DIF_LEFTTEXT),
        };

        static intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
        {
            if (msg == DN_CONTROLINPUT && param1 == 19)
            {
                const INPUT_RECORD *record=(const INPUT_RECORD*)param2;
                if (record->EventType==KEY_EVENT && record->Event.KeyEvent.bKeyDown)
                {
                    WORD Key=record->Event.KeyEvent.wVirtualKeyCode;
                    if (Key!=VK_ESCAPE)
                    {
                        wchar_t buf[32]{};
                        GetKeyNameTextW(record->Event.KeyEvent.wVirtualScanCode << 16, buf, sizeof(buf));

                        utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 19, (void*)buf);
                        return TRUE;
                    }
                }
            }

            return config::PsInfo.DefDlgProc(hdlg, msg, param1, param2);
        }

        bool ConfigDialog::show()
        {
            auto hdlg = config::PsInfo.DialogInit(&MainGuid, &ConfigDialogGuid, -1, -1, width, height, 0,
                &dlg_items_layout[0], std::size(dlg_items_layout), 0, FDLG_NONE, &dlg_proc, nullptr);

            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 1, (void*)L"Spotifar config");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 3, (void*)L"Client ID");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 4, (void*)utils::to_wstring(config::get_client_id()).c_str());
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 5, (void*)L"Client secret");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 6, (void*)utils::to_wstring(config::get_client_secret()).c_str());
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 7, (void*)L"Localhost port");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 8, (void*)utils::to_wstring(std::to_string(config::get_localhost_port())).c_str());
            
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 10, (void*)L"Ctrl");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 11, (void*)L"Shift");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 12, (void*)L"Alt");

            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 13, (void*)L"Play/pause");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 14, (void*)L"Space");

            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 18, (void*)L"Skip next");
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 19, (void*)L"Right");
            
            utils::far3::send_dlg_msg(hdlg, DM_SETTEXTPTR, 23, (void*)L"Global hotkeys");

            auto ExitCode=config::PsInfo.DialogRun(hdlg);
            if (ExitCode == 1) // OK
            {
                // различные вызовы Info.SendDlgMessage() для получения нужных выходных данных из диалога
            }
            config::PsInfo.DialogFree(hdlg);

            return false;
        }
    }
}