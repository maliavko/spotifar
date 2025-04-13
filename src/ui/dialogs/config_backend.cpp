#include "config_backend.hpp"
#include "config.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

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

    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    hotkeys_box_y = 1, // y position of a panel with hotkeys settings
    buttons_box_y = hotkeys_box_y + 9, // x position of a buttons panel
    width = 62, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2;

// TODO: localize strings
static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,                 DIF_NONE, L""),
    ctrl(DI_CHECKBOX,    view_center_x-8, hotkeys_box_y, view_center_x+8, 1, DIF_CENTERTEXT, L"Playback backend"),

    ctrl(DI_CHECKBOX,    view_x1, hotkeys_box_y+1, box_x1+10, 1,         DIF_CENTERTEXT, L"Vol. normalisation"),
    ctrl(DI_CHECKBOX,    view_x1, hotkeys_box_y+2, box_x1+10, 1,         DIF_CENTERTEXT, L"Autoplay similar"),
    ctrl(DI_CHECKBOX,    view_center_x+1, hotkeys_box_y+1, box_x1+10, 1, DIF_CENTERTEXT, L"Gapless playback"),
    ctrl(DI_CHECKBOX,    view_center_x+1, hotkeys_box_y+2, box_x1+10, 1, DIF_CENTERTEXT, L"Audio cache"),
    
    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,          DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,        DIF_CENTERGROUP | DIF_DEFAULTBUTTON, L"OK"),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,        DIF_CENTERGROUP, L"Cancel"),
};

config_backend_dialog::config_backend_dialog():
    modal_dialog(&ConfigBackendDialogGuid, width, height, dlg_items_layout)
{
    dialogs::set_checked(hdlg, backend_checkbox, config::is_playback_backend_enabled());
    dialogs::set_checked(hdlg, volume_normalisation_checkbox, config::is_playback_normalisation_enabled());
    dialogs::set_checked(hdlg, autoplay_checkbox, config::is_playback_autoplay_enabled());
    dialogs::set_checked(hdlg, gapless_playback_checkbox, config::is_gapless_playback_enabled());
    dialogs::set_checked(hdlg, playback_cache_checkbox, config::is_playback_cache_enabled());
}

intptr_t config_backend_dialog::handle_result(intptr_t dialog_run_result)
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

            ctx->fire_events(); // notify all the listeners
        }
        config::write();
        return TRUE;
    }
    return FALSE;
}

} // namespace ui
} // namespace spotifar