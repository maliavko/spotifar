#include "stdafx.h"
#include "config.hpp"
#include "utils.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar { namespace ui {

using namespace utils::far3;
namespace hotkeys = config::hotkeys;

enum controls : int
{
    no_control = -1,
    dialog_box,
    add_to_disk_checkbox,
    verbose_logging_checkbox,
    spotify_api_separator,
    api_client_id_label,
    api_client_id_edit,
    api_client_secret_label,
    api_client_secret_edit,
    api_port_label,
    api_port_edit,
    global_hotkeys_separator,
    hotkeys_checkbox,
    hotkeys_table_label,
    play_pause_hotkey_label,
    play_pause_hotkey_key,
    play_pause_hotkey_char,
    play_pause_hotkey_ctrl,
    play_pause_hotkey_shift,
    play_pause_hotkey_alt,
    skip_next_hotkey_label,
    skip_next_hotkey_key,
    skip_next_hotkey_char,
    skip_next_hotkey_ctrl,
    skip_next_hotkey_shift,
    skip_next_hotkey_alt,
    skip_prev_hotkey_label,
    skip_prev_hotkey_key,
    skip_prev_hotkey_char,
    skip_prev_hotkey_ctrl,
    skip_prev_hotkey_shift,
    skip_prev_hotkey_alt,
    seek_forward_hotkey_label,
    seek_forward_hotkey_key,
    seek_forward_hotkey_char,
    seek_forward_hotkey_ctrl,
    seek_forward_hotkey_shift,
    seek_forward_hotkey_alt,
    seek_backward_hotkey_label,
    seek_backward_hotkey_key,
    seek_backward_hotkey_char,
    seek_backward_hotkey_ctrl,
    seek_backward_hotkey_shift,
    seek_backward_hotkey_alt,
    volume_up_hotkey_label,
    volume_up_hotkey_key,
    volume_up_hotkey_char,
    volume_up_hotkey_ctrl,
    volume_up_hotkey_shift,
    volume_up_hotkey_alt,
    volume_down_hotkey_label,
    volume_down_hotkey_key,
    volume_down_hotkey_char,
    volume_down_hotkey_ctrl,
    volume_down_hotkey_shift,
    volume_down_hotkey_alt,
    buttons_separator,
    ok_button,
    cancel_button,
};

static const std::unordered_map<controls, int> hotkey_edits{
    { play_pause_hotkey_key, hotkeys::play },
    { skip_next_hotkey_key, hotkeys::skip_next },
    { skip_prev_hotkey_key, hotkeys::skip_previous },
    { seek_forward_hotkey_key, hotkeys::seek_forward },
    { seek_backward_hotkey_key, hotkeys::seek_forward },
    { volume_up_hotkey_key, hotkeys::volume_up },
    { volume_down_hotkey_key, hotkeys::volume_down },
};

static const int
    api_box_y = 5, // x position of a panel with api settings
    hotkeys_box_y = api_box_y + 4, // x position of a panel with hotkeys settings
    buttons_box_y = hotkeys_box_y + 9, // x position of a buttons panel
    width = 62, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    box_x1 = 3, box_x2 = width - 4, box_y1 = 1, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2;

static FarDialogItem control(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
                             FARDIALOGITEMFLAGS flags, const wchar_t *data = L"")
{
    return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
}

static const std::vector<FarDialogItem> dlg_items_layout{
    control(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,                 DIF_NONE, L"Spotifar settings"),

    // global settings
    control(DI_CHECKBOX,    view_x1, view_y1, view_x1 + 15, 1,              DIF_LEFTTEXT, L"Add to disk menu"),
    control(DI_CHECKBOX,    view_x1, view_y1+1, view_x1 + 15, 1,            DIF_LEFTTEXT, L"Verbose logging"),
    
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

/// @brief The function `GetKeyNameTextW` works no great, for some corner cases
/// it returns empty or error translation. Plus the text will depend on the 
/// locale language selected by user in OS. The function tries to be not
/// dependent on these problems
static wstring get_key_name(WORD virtual_key_code)
{
    // if the key is a text key
    if (virtual_key_code >= utils::far3::keys::a && virtual_key_code <= utils::far3::keys::z)
        return wstring(1, (char)virtual_key_code);

    // if it is a special key
    switch (virtual_key_code)
    {
        case VK_DOWN: return L"Down";
        case VK_LEFT: return L"Left";
        case VK_RIGHT: return L"Right";
        case VK_UP: return L"Up";
        case VK_END: return L"End";
        case VK_NAVIGATION_DOWN: return L"Pg Down";
        case VK_NAVIGATION_UP: return L"Pg Up";
        case VK_CAPITAL: return L"Caps";
        case VK_OEM_PERIOD: return L".";
        case VK_OEM_COMMA: return L",";
        case VK_OEM_1: return L";";
        case VK_OEM_2: return L"/";
        case VK_OEM_3: return L"";
        case VK_OEM_4: return L"[";
        case VK_OEM_5: return L"\\";
        case VK_OEM_6: return L"]";
        case VK_OEM_7: return L"'";
    }
    
    // the rest we'll try to map via winapi
    wchar_t buf[32]{};
    auto scan_code = MapVirtualKeyA(virtual_key_code, MAPVK_VK_TO_VSC);
    GetKeyNameTextW(scan_code << 16, buf, sizeof(buf));

    return buf;
}

static intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
{
    if (msg == DN_CONTROLINPUT && hotkey_edits.contains((controls)param1))
    {
        auto *record = (const INPUT_RECORD*)param2;
        auto &key_event = record->Event.KeyEvent;
        if (record->EventType == KEY_EVENT && key_event.bKeyDown)
        {
            int key = input_record_to_combined_key(key_event), edit_id = (int)param1;
            if (key == VK_BACK)
            {
                msg::set_text(hdlg, edit_id, L"");
                msg::set_text(hdlg, edit_id + 1, L"0");
                msg::set_checked(hdlg, edit_id + 2, false);
                msg::set_checked(hdlg, edit_id + 3, false);
                msg::set_checked(hdlg, edit_id + 4, false);
                return TRUE;
            }
            else if (key != VK_ESCAPE)
            {
                auto key_name = get_key_name(key_event.wVirtualKeyCode);
                auto key_code = std::to_wstring(key_event.wVirtualKeyCode);

                // disallowing to pick the already selected key
                for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
                    if (msg::get_text(hdlg, ctrl_id) == key_name)
                        return TRUE;

                msg::set_text(hdlg, edit_id, key_name);
                msg::set_text(hdlg, edit_id + 1, key_code);
                return TRUE;
            }
        }
    }

    return config::ps_info.DefDlgProc(hdlg, msg, param1, param2);
}

bool ConfigDialog::show()
{
    auto hdlg = config::ps_info.DialogInit(&MainGuid, &ConfigDialogGuid, -1, -1, width, height, 0,
        &dlg_items_layout[0], std::size(dlg_items_layout), 0, FDLG_NONE, &dlg_proc, nullptr);

    msg::set_checked(hdlg, add_to_disk_checkbox, config::is_added_to_disk_menu());
    msg::set_checked(hdlg, verbose_logging_checkbox, config::is_verbose_logging_enabled());
    msg::set_checked(hdlg, hotkeys_checkbox, config::is_global_hotkeys_enabled());
    msg::set_text(hdlg, api_client_id_edit, config::get_client_id());
    msg::set_text(hdlg, api_client_secret_edit, config::get_client_secret());
    msg::set_text(hdlg, api_port_edit, std::to_string(config::get_localhost_port()));

    for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
    {
        auto key_and_mods = config::get_hotkey(hotkey_id);
        if (key_and_mods == nullptr)
            continue;

        msg::set_text(hdlg, ctrl_id, get_key_name(key_and_mods->first));
        msg::set_text(hdlg, ctrl_id + 1, std::to_wstring(key_and_mods->first));
        msg::set_checked(hdlg, ctrl_id + 2, key_and_mods->second & MOD_CONTROL);
        msg::set_checked(hdlg, ctrl_id + 3, key_and_mods->second & MOD_SHIFT);
        msg::set_checked(hdlg, ctrl_id + 4, key_and_mods->second & MOD_ALT);
    }

    if (config::ps_info.DialogRun(hdlg) == ok_button)
    {
        {
            auto ctx = config::lock_settings();
            auto &s = ctx->get_settings();

            s.add_to_disk_menu = msg::is_checked(hdlg, add_to_disk_checkbox);
            s.verbose_logging = msg::is_checked(hdlg, verbose_logging_checkbox);
            s.is_global_hotkeys_enabled = msg::is_checked(hdlg, hotkeys_checkbox);
            s.spotify_client_id = msg::get_text(hdlg, api_client_id_edit);
            s.spotify_client_secret = msg::get_text(hdlg, api_client_secret_edit);
            s.localhost_service_port = std::stoi(msg::get_text(hdlg, api_port_edit));

            for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
            {
                WORD key = std::stoi(msg::get_text(hdlg, ctrl_id + 1)), mods = 0;
                if (msg::is_checked(hdlg, ctrl_id + 2)) mods |= MOD_CONTROL;
                if (msg::is_checked(hdlg, ctrl_id + 3)) mods |= MOD_SHIFT;
                if (msg::is_checked(hdlg, ctrl_id + 4)) mods |= MOD_ALT;
                s.hotkeys[hotkey_id] = std::make_pair(key, mods);
            }
        }
        config::write();
    }
    config::ps_info.DialogFree(hdlg);

    return false;
}

} // namespace ui
} // namespace spotifar