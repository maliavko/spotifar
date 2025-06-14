#include "backend.hpp"
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

    backend_checkbox,
    volume_normalisation_checkbox,
    autoplay_checkbox,
    gapless_playback_checkbox,
    playback_cache_checkbox,

    bitrate_label,
    bitrate_combo,
    format_label,
    format_combo,
    dither_label,
    dither_combo,
    volume_ctrl_label,
    volume_ctrl_combo,

    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    main_box_y = 2, // y position of a panel with hotkeys settings
    buttons_box_y = main_box_y + 7, // y position of a buttons panel
    width = 62, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2,
    col2_x = view_center_x + 2;

static const auto combo_flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,            DIF_NONE),

    ctrl(DI_CHECKBOX,    view_center_x-8, 1, view_center_x+8, 1,    DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1, main_box_y+1, view_x1+10, 1,      DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    view_x1, main_box_y+2, view_x1+10, 1,      DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    col2_x, main_box_y+1, col2_x+10, 1,        DIF_CENTERTEXT),
    ctrl(DI_CHECKBOX,    col2_x, main_box_y+2, col2_x+10, 1,        DIF_CENTERTEXT),
    
    ctrl(DI_TEXT,        view_x1, main_box_y+4, view_x1+10, 1,      DIF_NONE),
    ctrl(DI_COMBOBOX,    view_x1+11, main_box_y+4, view_x1+21, 1,   combo_flags),
    ctrl(DI_TEXT,        view_x1, main_box_y+5, view_x1+10, 1,      DIF_NONE),
    ctrl(DI_COMBOBOX,    view_x1+11, main_box_y+5, view_x1+21, 1,   combo_flags),
    ctrl(DI_TEXT,        col2_x, main_box_y+4, col2_x+10, 1,        DIF_NONE),
    ctrl(DI_COMBOBOX,    col2_x+13, main_box_y+4, col2_x+23, 1,     combo_flags),
    ctrl(DI_TEXT,        col2_x, main_box_y+5, col2_x+10, 1,        DIF_NONE),
    ctrl(DI_COMBOBOX,    col2_x+13, main_box_y+5, col2_x+23, 1,     combo_flags),
    
    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,     DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,   DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,   DIF_CENTERGROUP),
};

static void populate_combobox(HANDLE hdlg, int combo_id, std::vector<string> items, const string &current)
{
    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        dialogs::add_list_item(hdlg, combo_id, utils::to_wstring(item), (int)idx,
            (void*)item.c_str(), item.size(), item == current);
    }
}

backend_dialog::backend_dialog():
    modal_dialog(&ConfigBackendDialogGuid, width, height, dlg_items_layout)
{
    dialogs::set_checked(hdlg, backend_checkbox, config::is_playback_backend_enabled());
    dialogs::set_checked(hdlg, volume_normalisation_checkbox, config::is_playback_normalisation_enabled());
    dialogs::set_checked(hdlg, autoplay_checkbox, config::is_playback_autoplay_enabled());
    dialogs::set_checked(hdlg, gapless_playback_checkbox, config::is_gapless_playback_enabled());
    dialogs::set_checked(hdlg, playback_cache_checkbox, config::is_playback_cache_enabled());
}

void backend_dialog::init()
{
    // comboboxes initialization
    populate_combobox(hdlg, bitrate_combo, config::playback::bitrate::all, config::get_playback_bitrate());
    populate_combobox(hdlg, format_combo, config::playback::format::all, config::get_playback_format());
    populate_combobox(hdlg, dither_combo, config::playback::dither::all, config::get_playback_dither());
    populate_combobox(hdlg, volume_ctrl_combo, config::playback::volume_ctrl::all, config::get_playback_volume_ctrl());

    dialogs::set_text(hdlg, backend_checkbox, get_text(MCfgPlayback));
    dialogs::set_text(hdlg, volume_normalisation_checkbox, get_text(MCfgNormalisation));
    dialogs::set_text(hdlg, autoplay_checkbox, get_text(MCfgAutoplay));
    dialogs::set_text(hdlg, gapless_playback_checkbox, get_text(MCfgGapless));
    dialogs::set_text(hdlg, playback_cache_checkbox, get_text(MCfgCache));
    dialogs::set_text(hdlg, bitrate_label, get_text(MCfgBitrate));
    dialogs::set_text(hdlg, format_label, get_text(MCfgFormat));
    dialogs::set_text(hdlg, dither_label, get_text(MCfgDither));
    dialogs::set_text(hdlg, volume_ctrl_label, get_text(MCfgVolumeCtrl));
    dialogs::set_text(hdlg, ok_button, get_text(MOk));
    dialogs::set_text(hdlg, cancel_button, get_text(MCancel));
}

intptr_t backend_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_button)
    {
        {
            auto ctx = config::lock_settings();
            auto &s = ctx->get_settings();

            s.playback_backend_enabled = dialogs::is_checked(hdlg, backend_checkbox);
            s.volume_normalisation_enabled = dialogs::is_checked(hdlg, volume_normalisation_checkbox);
            s.playback_autoplay_enabled = dialogs::is_checked(hdlg, autoplay_checkbox);
            s.gapless_playback_enabled = dialogs::is_checked(hdlg, gapless_playback_checkbox);
            s.playback_cache_enabled = dialogs::is_checked(hdlg, playback_cache_checkbox);

            s.playback_bitrate = dialogs::get_list_current_item_data<string>(hdlg, bitrate_combo);
            s.playback_format = dialogs::get_list_current_item_data<string>(hdlg, format_combo);
            s.playback_dither = dialogs::get_list_current_item_data<string>(hdlg, dither_combo);
            s.playback_volume_ctrl = dialogs::get_list_current_item_data<string>(hdlg, volume_ctrl_combo);

            ctx->fire_events(); // notify all the listeners
        }
        config::write();
        return TRUE;
    }
    return FALSE;
}

} // namespace settings
} // namespace ui
} // namespace spotifar