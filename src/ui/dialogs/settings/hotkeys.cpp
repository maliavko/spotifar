#include "hotkeys.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"

namespace spotifar { namespace ui { namespace settings {

using namespace utils::far3;
namespace hotkeys = config::hotkeys;

enum controls : int
{
    no_control = -1,
    dialog_box,
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

    show_toast_hotkey_label,
    show_toast_hotkey_key,
    show_toast_hotkey_char,
    show_toast_hotkey_ctrl,
    show_toast_hotkey_shift,
    show_toast_hotkey_alt,

    buttons_separator,
    ok_button,
    cancel_button,
};

static const std::unordered_map<controls, int> hotkey_edits{
    { play_pause_hotkey_key, hotkeys::play },
    { skip_next_hotkey_key, hotkeys::skip_next },
    { skip_prev_hotkey_key, hotkeys::skip_previous },
    { seek_forward_hotkey_key, hotkeys::seek_forward },
    { seek_backward_hotkey_key, hotkeys::seek_backward },
    { volume_up_hotkey_key, hotkeys::volume_up },
    { volume_down_hotkey_key, hotkeys::volume_down },
    { show_toast_hotkey_key, hotkeys::show_toast },
};

static const int
    hotkeys_box_y = 1, // y position of a panel with hotkeys settings
    buttons_box_y = hotkeys_box_y + 10, // y position of a buttons panel
    width = 62, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,                 DIF_NONE),
    ctrl(DI_CHECKBOX,    view_center_x-8, hotkeys_box_y, view_center_x+8, 1, DIF_CENTERTEXT),
    ctrl(DI_TEXT,        view_x1+26, hotkeys_box_y+1, view_x2, 1,        DIF_LEFTTEXT),

    // play/pause hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+2, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+2, view_x1+22, 1,     DIF_CENTERTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+2, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+2, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+2, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+2, 1, 1,              DIF_LEFTTEXT),
    // skip to next hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+3, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+3, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+3, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+3, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+3, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+3, 1, 1,              DIF_LEFTTEXT),
    // skip to prev hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+4, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+4, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+4, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+4, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+4, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+4, 1, 1,              DIF_LEFTTEXT),
    // seek forward hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+5, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+5, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+5, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+5, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+5, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+5, 1, 1,              DIF_LEFTTEXT),
    // seek backward hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+6, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+6, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+6, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+6, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+6, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+6, 1, 1,              DIF_LEFTTEXT),
    // volume up hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+7, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+7, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+7, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+7, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+7, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+7, 1, 1,              DIF_LEFTTEXT),
    // volume down hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+8, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+8, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+8, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+8, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+8, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+8, 1, 1,              DIF_LEFTTEXT),
    // show toast hotkey
    ctrl(DI_TEXT,        view_x1, hotkeys_box_y+9, view_x1+15, 1,        DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, hotkeys_box_y+9, view_x1+22, 1,     DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1+23, hotkeys_box_y+9, view_x1+32, 1,     DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1+34, hotkeys_box_y+9, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+41, hotkeys_box_y+9, 1, 1,              DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+48, hotkeys_box_y+9, 1, 1,              DIF_LEFTTEXT),

    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,          DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,        DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,        DIF_CENTERGROUP),
};

hotkeys_dialog::hotkeys_dialog():
    modal_dialog(&ConfigHotkeysDialogGuid, width, height, dlg_items_layout)
{
    dialogs::set_checked(hdlg, hotkeys_checkbox, config::is_global_hotkeys_enabled());

    for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
    {
        auto key_and_mods = config::get_hotkey(hotkey_id);
        if (key_and_mods == nullptr)
            continue;

        dialogs::set_text(hdlg, ctrl_id, utils::keys::vk_to_string(key_and_mods->first));
        dialogs::set_text(hdlg, ctrl_id + 1, std::to_wstring(key_and_mods->first));
        dialogs::set_checked(hdlg, ctrl_id + 2, key_and_mods->second & MOD_CONTROL);
        dialogs::set_checked(hdlg, ctrl_id + 3, key_and_mods->second & MOD_SHIFT);
        dialogs::set_checked(hdlg, ctrl_id + 4, key_and_mods->second & MOD_ALT);
    }
}

void hotkeys_dialog::init()
{
    dialogs::set_text(hdlg, hotkeys_checkbox, get_text(MConfigHotkeysBoxLabel));
    dialogs::set_text(hdlg, hotkeys_table_label, get_text(MConfigHotkeysTableTitle));
    dialogs::set_text(hdlg, play_pause_hotkey_label, get_text(MConfigPlayPauseSetting));
    dialogs::set_text(hdlg, skip_next_hotkey_label, get_text(MConfigSkipToNextSetting));
    dialogs::set_text(hdlg, skip_prev_hotkey_label, get_text(MConfigSkipToPrevSetting));
    dialogs::set_text(hdlg, seek_forward_hotkey_label, get_text(MConfigSeekForwardSetting));
    dialogs::set_text(hdlg, seek_backward_hotkey_label, get_text(MConfigSeekBackwardSetting));
    dialogs::set_text(hdlg, volume_up_hotkey_label, get_text(MConfigVolumeUpSetting));
    dialogs::set_text(hdlg, volume_down_hotkey_label, get_text(MConfigVolumeDownSetting));
    dialogs::set_text(hdlg, show_toast_hotkey_label, get_text(MConfigShowToastSetting));

    dialogs::set_text(hdlg, ok_button, get_text(MOk));
    dialogs::set_text(hdlg, cancel_button, get_text(MCancel));
}

intptr_t hotkeys_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_button)
    {
        {
            auto ctx = config::lock_settings();
            auto &s = ctx->get_settings();

            s.is_global_hotkeys_enabled = dialogs::is_checked(hdlg, hotkeys_checkbox);

            for (const auto &[ctrl_id, hotkey_id]: hotkey_edits)
            {
                WORD key = std::stoi(dialogs::get_text(hdlg, ctrl_id + 1)), mods = 0;
                if (dialogs::is_checked(hdlg, ctrl_id + 2)) mods |= MOD_CONTROL;
                if (dialogs::is_checked(hdlg, ctrl_id + 3)) mods |= MOD_SHIFT;
                if (dialogs::is_checked(hdlg, ctrl_id + 4)) mods |= MOD_ALT;
                s.hotkeys[hotkey_id] = std::make_pair(key, mods);
            }

            ctx->fire_events(); // notify all the listeners
        }
        config::write();
        return TRUE;
    }
    return FALSE;
}

bool hotkeys_dialog::handle_key_pressed(int ctrl_id, int combined_key)
{
    // processing only key edit boxes
    if (!hotkey_edits.contains((controls)ctrl_id))
        return false;
    
    // clearing currently selected editbox and all related checkboxes
    if (combined_key == VK_BACK)
    {
        dialogs::set_text(hdlg, ctrl_id, L"");
        dialogs::set_text(hdlg, ctrl_id + 1, L"0");
        dialogs::set_checked(hdlg, ctrl_id + 2, false);
        dialogs::set_checked(hdlg, ctrl_id + 3, false);
        dialogs::set_checked(hdlg, ctrl_id + 4, false);
        return TRUE;
    }

    // Esc closes the dialog, all the rest keys can be used as hotkeys
    if (combined_key != VK_ESCAPE)
    {
        auto vk_code = LOWORD(combined_key);
        auto key_name = utils::keys::vk_to_string(vk_code);
        auto key_code = std::to_wstring(vk_code);

        // disallowing to pick the already selected key
        for (auto &[ctrl_id, hotkey_id]: hotkey_edits)
            if (dialogs::get_text(hdlg, ctrl_id) == key_name)
                return TRUE;

        dialogs::set_text(hdlg, ctrl_id, key_name);
        dialogs::set_text(hdlg, ctrl_id + 1, key_code);
        return TRUE;
    }
    return FALSE;
}

} // namespace settings
} // namespace ui
} // namespace spotifar