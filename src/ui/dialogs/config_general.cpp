#include "config_general.hpp"
#include "config.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

using namespace utils::far3;

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
    
    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    api_box_y = 5, // y position of a panel with api settings
    buttons_box_y = api_box_y + 4, // y position of a buttons panel
    width = 62, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,             DIF_NONE, L"General"),

    // global settings
    ctrl(DI_CHECKBOX,    view_x1, view_y1, view_x1 + 15, 1,          DIF_LEFTTEXT, L"Add to disk menu"),
    ctrl(DI_CHECKBOX,    view_x1, view_y1+1, view_x1 + 15, 1,        DIF_LEFTTEXT, L"Verbose logging"),
    
    // api settings
    ctrl(DI_TEXT,        -1, api_box_y, box_x2, box_y2,              DIF_SEPARATOR, L"Spotify API"),
    ctrl(DI_TEXT,        view_x1, api_box_y+1, view_x1+15, 1,        DIF_LEFTTEXT, L"Client ID"),
    ctrl(DI_EDIT,        view_x1+15, api_box_y+1, view_x2, 1,        DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1, api_box_y+2, view_x1+15, 1,        DIF_LEFTTEXT, L"Client secret"),
    ctrl(DI_EDIT,        view_x1+15, api_box_y+2, view_x2, 1,        DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1, api_box_y+3, view_x1+15, 1,        DIF_LEFTTEXT, L"Localhost port"),
    ctrl(DI_EDIT,        view_x1+15, api_box_y+3, view_x2, 1,        DIF_LEFTTEXT),

    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,      DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,    DIF_CENTERGROUP | DIF_DEFAULTBUTTON, L"OK"),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,    DIF_CENTERGROUP, L"Cancel"),
};

config_general_dialog::config_general_dialog():
    modal_dialog(&ConfigGeneralDialogGuid, width, height, dlg_items_layout)
{
    dialogs::set_checked(hdlg, add_to_disk_checkbox, config::is_added_to_disk_menu());
    dialogs::set_checked(hdlg, verbose_logging_checkbox, config::is_verbose_logging_enabled());
    dialogs::set_text(hdlg, api_client_id_edit, config::get_client_id());
    dialogs::set_text(hdlg, api_client_secret_edit, config::get_client_secret());
    dialogs::set_text(hdlg, api_port_edit, std::to_string(config::get_localhost_port()));
}

intptr_t config_general_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_button)
    {
        {
            auto ctx = config::lock_settings();
            auto &s = ctx->get_settings();

            s.add_to_disk_menu = dialogs::is_checked(hdlg, add_to_disk_checkbox);
            s.verbose_logging = dialogs::is_checked(hdlg, verbose_logging_checkbox);
            s.spotify_client_id = dialogs::get_text(hdlg, api_client_id_edit);
            s.spotify_client_secret = dialogs::get_text(hdlg, api_client_secret_edit);
            s.localhost_service_port = std::stoi(dialogs::get_text(hdlg, api_port_edit));

            ctx->fire_events(); // notify all the listeners
        }
        config::write();
    }
    return FALSE;
}

} // namespace ui
} // namespace spotifar
