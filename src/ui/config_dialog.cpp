#include "stdafx.h"
#include "config.hpp"
#include "utils.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
    namespace ui
    {
        using config::HotkeyID;
        using namespace utils::far3;

        enum DialogControls
        {
            NO_CONTROL = -1,
            DIALOG_BOX,
            ADD_TO_DISK_CHECKBOX,
            SPOTIFY_API_SEPARATOR,
            API_CLIENT_ID_LABEL,
            API_CLIENT_ID_EDIT,
            API_CLIENT_SECRET_LABEL,
            API_CLIENT_SECRET_EDIT,
            API_PORT_LABEL,
            API_PORT_EDIT,
            GLOBAL_HOTKEYS_SEPARATOR,
            HOTKEYS_CHECKBOX,
            HOTKEYS_TABLE_LABEL,
            PLAY_PAUSE_HOTKEY_LABEL,
            PLAY_PAUSE_HOTKEY_KEY,
            PLAY_PAUSE_HOTKEY_CHAR,
            PLAY_PAUSE_HOTKEY_CTRL,
            PLAY_PAUSE_HOTKEY_SHIFT,
            PLAY_PAUSE_HOTKEY_ALT,
            SKIP_NEXT_HOTKEY_LABEL,
            SKIP_NEXT_HOTKEY_KEY,
            SKIP_NEXT_HOTKEY_CHAR,
            SKIP_NEXT_HOTKEY_CTRL,
            SKIP_NEXT_HOTKEY_SHIFT,
            SKIP_NEXT_HOTKEY_ALT,
            SKIP_PREV_HOTKEY_LABEL,
            SKIP_PREV_HOTKEY_KEY,
            SKIP_PREV_HOTKEY_CHAR,
            SKIP_PREV_HOTKEY_CTRL,
            SKIP_PREV_HOTKEY_SHIFT,
            SKIP_PREV_HOTKEY_ALT,
            SEEK_FORWARD_HOTKEY_LABEL,
            SEEK_FORWARD_HOTKEY_KEY,
            SEEK_FORWARD_HOTKEY_CHAR,
            SEEK_FORWARD_HOTKEY_CTRL,
            SEEK_FORWARD_HOTKEY_SHIFT,
            SEEK_FORWARD_HOTKEY_ALT,
            SEEK_BACKWARD_HOTKEY_LABEL,
            SEEK_BACKWARD_HOTKEY_KEY,
            SEEK_BACKWARD_HOTKEY_CHAR,
            SEEK_BACKWARD_HOTKEY_CTRL,
            SEEK_BACKWARD_HOTKEY_SHIFT,
            SEEK_BACKWARD_HOTKEY_ALT,
            VOLUME_UP_HOTKEY_LABEL,
            VOLUME_UP_HOTKEY_KEY,
            VOLUME_UP_HOTKEY_CHAR,
            VOLUME_UP_HOTKEY_CTRL,
            VOLUME_UP_HOTKEY_SHIFT,
            VOLUME_UP_HOTKEY_ALT,
            VOLUME_DOWN_HOTKEY_LABEL,
            VOLUME_DOWN_HOTKEY_KEY,
            VOLUME_DOWN_HOTKEY_CHAR,
            VOLUME_DOWN_HOTKEY_CTRL,
            VOLUME_DOWN_HOTKEY_SHIFT,
            VOLUME_DOWN_HOTKEY_ALT,
            BUTTONS_SEPARATOR,
            OK_BUTTON,
            CANCEL_BUTTON,
        };

        static std::unordered_map<DialogControls, HotkeyID> hotkey_edits{
            { PLAY_PAUSE_HOTKEY_KEY, HotkeyID::PLAY },
            { SKIP_NEXT_HOTKEY_KEY, HotkeyID::SKIP_NEXT },
            { SKIP_PREV_HOTKEY_KEY, HotkeyID::SKIP_PREV },
            { SEEK_FORWARD_HOTKEY_KEY, HotkeyID::SEEK_FORWARD },
            { SEEK_BACKWARD_HOTKEY_KEY, HotkeyID::SEEK_BACKWARD },
            { VOLUME_UP_HOTKEY_KEY, HotkeyID::VOLUME_UP },
            { VOLUME_DOWN_HOTKEY_KEY, HotkeyID::VOLUME_DOWN },
        };

        static const int width = 62, height = 21;
        static const int box_x1 = 3, box_x2 = width - 4, box_y1 = 1, box_y2 = height - 2;
        static const int view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;
        static const int view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2;
        static const int api_box_y = 3, hotkeys_box_y = 8, buttons_box_y = 17;

        static FarDialogItem control(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
                                     FARDIALOGITEMFLAGS flags, const wchar_t *data = L"")
        {
            return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
        }
    
        static std::vector<FarDialogItem> dlg_items_layout{
            control(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,                 DIF_NONE, L"Spotifar settings"),

            // global settings
            control(DI_CHECKBOX,    view_x1, view_y1, view_x1 + 15, 1,              DIF_LEFTTEXT, L"Add to disk menu"),
            
            // api settings
            control(DI_TEXT,        box_x1, api_box_y, box_x2, box_y2,              DIF_SEPARATOR),
            control(DI_TEXT,        view_x1, api_box_y+1, view_x1+15, 1,            DIF_LEFTTEXT, L"Client ID"),
            control(DI_EDIT,        view_x1+15, api_box_y+1, view_x2, 1,            DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1, api_box_y+2, view_x1+15, 1,            DIF_LEFTTEXT, L"Client secret"),
            control(DI_EDIT,        view_x1+15, api_box_y+2, view_x2, 1,            DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1, api_box_y+3, view_x1+15, 1,            DIF_LEFTTEXT, L"Localhost port"),
            control(DI_EDIT,        view_x1+15, api_box_y+3, view_x2, 1,            DIF_LEFTTEXT),

            // global hotkeys settings block   
            control(DI_TEXT,        box_x1, hotkeys_box_y, box_x2, box_y2,          DIF_SEPARATOR),         
            control(DI_CHECKBOX,    view_center_x-8, hotkeys_box_y, view_center_x+8, 1, DIF_CENTERTEXT, L"Global hotkeys"),
            control(DI_TEXT,        view_x1+26, hotkeys_box_y+1, view_x2, 1,        DIF_LEFTTEXT, L"VKey    Ctrl  Shift   Alt"),
            // play/pause hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+2, view_x1+15, 1,        DIF_LEFTTEXT, L"play/pause"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+2, view_x1+22, 1,     DIF_CENTERTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+2, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+2, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+2, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+2, 1, 1,              DIF_LEFTTEXT),
            // skip to next hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+3, view_x1+15, 1,        DIF_LEFTTEXT, L"skip to next"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+3, view_x1+22, 1,     DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+3, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+3, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+3, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+3, 1, 1,              DIF_LEFTTEXT),
            // skip to prev hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+4, view_x1+15, 1,        DIF_LEFTTEXT, L"skip to prev"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+4, view_x1+22, 1,     DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+4, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+4, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+4, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+4, 1, 1,              DIF_LEFTTEXT),
            // seek forward hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+5, view_x1+15, 1,        DIF_LEFTTEXT, L"seek forward"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+5, view_x1+22, 1,     DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+5, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+5, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+5, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+5, 1, 1,              DIF_LEFTTEXT),
            // seek backward hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+6, view_x1+15, 1,        DIF_LEFTTEXT, L"seek backward"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+6, view_x1+22, 1,     DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+6, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+6, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+6, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+6, 1, 1,              DIF_LEFTTEXT),
            // volume up hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+7, view_x1+15, 1,        DIF_LEFTTEXT, L"volume up"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+7, view_x1+22, 1,     DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+7, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+7, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+7, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+7, 1, 1,              DIF_LEFTTEXT),
            // volume down hotkey
            control(DI_TEXT,        view_x1, hotkeys_box_y+8, view_x1+15, 1,        DIF_LEFTTEXT, L"volume down"),
            control(DI_EDIT,        view_x1+15, hotkeys_box_y+8, view_x1+22, 1,     DIF_LEFTTEXT),
            control(DI_TEXT,        view_x1+23, hotkeys_box_y+8, view_x1+32, 1,     DIF_CENTERTEXT),
            control(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+8, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+8, 1, 1,              DIF_LEFTTEXT),
            control(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+8, 1, 1,              DIF_LEFTTEXT),

            // buttons block
            control(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,          DIF_SEPARATOR),
            control(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,        DIF_CENTERGROUP | DIF_DEFAULTBUTTON, L"OK"),
            control(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,        DIF_CENTERGROUP, L"Cancel"),

        };

        static wstring get_key_name(WORD virtual_key_code)
        {
            wchar_t buf[32]{};
            auto scan_code = MapVirtualKeyA(virtual_key_code, MAPVK_VK_TO_VSC);
            GetKeyNameTextW(scan_code << 16, buf, sizeof(buf));

            if (!wcscmp(buf, L"Num 2")) return L"Down";
            if (!wcscmp(buf, L"Num 4")) return L"Left";
            if (!wcscmp(buf, L"Num 6")) return L"Right";
            if (!wcscmp(buf, L"Num 8")) return L"Up";
            if (!wcscmp(buf, L"Num 1")) return L"End";
            if (!wcscmp(buf, L"Num 3")) return L"Pg Down";
            if (!wcscmp(buf, L"Num 9")) return L"Pg Up";
            
            return buf;
        }

        static intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
        {
            if (msg == DN_CONTROLINPUT && hotkey_edits.contains((DialogControls)param1))
            {
                auto *record = (const INPUT_RECORD*)param2;
                auto &key_event = record->Event.KeyEvent;
                if (record->EventType == KEY_EVENT && key_event.bKeyDown)
                {
                    int key = input_record_to_combined_key(key_event), edit_id = (int)param1;
                    if (key == VK_BACK)
                    {
                        set_textptr(hdlg, edit_id, L"");
                        set_textptr(hdlg, edit_id + 1, L"0");
                        set_checkbox(hdlg, edit_id + 2, false);
                        set_checkbox(hdlg, edit_id + 3, false);
                        set_checkbox(hdlg, edit_id + 4, false);
                        return TRUE;
                    }
                    else if (key != VK_ESCAPE)
                    {
                        auto key_name = get_key_name(key_event.wVirtualKeyCode);
                        auto key_code = std::to_wstring(key_event.wVirtualKeyCode);

                        // disallowing to pick the already selected key
                        for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
                            if (get_textptr(hdlg, ctrl_id) == key_name)
                                return TRUE;

                        set_textptr(hdlg, edit_id, key_name);
                        set_textptr(hdlg, edit_id + 1, key_code);
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

            set_checkbox(hdlg, ADD_TO_DISK_CHECKBOX, config::is_added_to_disk_menu());
            set_checkbox(hdlg, HOTKEYS_CHECKBOX, config::is_global_hotkeys_enabled());
            set_textptr(hdlg, API_CLIENT_ID_EDIT, config::get_client_id());
            set_textptr(hdlg, API_CLIENT_SECRET_EDIT, config::get_client_secret());
            set_textptr(hdlg, API_PORT_EDIT, std::to_string(config::get_localhost_port()));

            for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
            {
                auto key_and_mods = config::get_hotkey(hotkey_id);
                if (key_and_mods == nullptr)
                    continue;

                set_textptr(hdlg, ctrl_id, get_key_name(key_and_mods->first));
                set_textptr(hdlg, ctrl_id + 1, std::to_wstring(key_and_mods->first));
                set_checkbox(hdlg, ctrl_id + 2, key_and_mods->second & MOD_CONTROL);
                set_checkbox(hdlg, ctrl_id + 3, key_and_mods->second & MOD_SHIFT);
                set_checkbox(hdlg, ctrl_id + 4, key_and_mods->second & MOD_ALT);
            }

            if (config::PsInfo.DialogRun(hdlg) == OK_BUTTON)
            {
                {
                    auto ctx = config::lock_settings();
                    auto &s = ctx->get_settings();

                    s.add_to_disk_menu = get_checkbox(hdlg, ADD_TO_DISK_CHECKBOX);
                    s.is_global_hotkeys_enabled = get_checkbox(hdlg, HOTKEYS_CHECKBOX);
                    s.spotify_client_id = get_textptr(hdlg, API_CLIENT_ID_EDIT);
                    s.spotify_client_secret = get_textptr(hdlg, API_CLIENT_SECRET_EDIT);
                    s.localhost_service_port = std::stoi(get_textptr(hdlg, API_PORT_EDIT));

                    for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
                    {
                        WORD key = std::stoi(get_textptr(hdlg, ctrl_id + 1)), mods = 0;
                        if (get_checkbox(hdlg, ctrl_id + 2)) mods |= MOD_CONTROL;
                        if (get_checkbox(hdlg, ctrl_id + 3)) mods |= MOD_SHIFT;
                        if (get_checkbox(hdlg, ctrl_id + 4)) mods |= MOD_ALT;
                        s.hotkeys[hotkey_id] = std::make_pair(key, mods);
                    }
                }
			    config::write();
            }
            config::PsInfo.DialogFree(hdlg);

            return false;
        }
    }
}