#include "general.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"

namespace spotifar { namespace ui { namespace settings {

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
    
    ui_separator,
    track_changed_notifications,
    image_shape_label,
    image_shape_combo,
    
    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    api_box_y = 5, // y position of the api settings block
    ui_box_y = api_box_y + 5, // y position of the UI settings block
    buttons_box_y = ui_box_y + 3, // y position of a buttons block
    width = 62, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    view_center_x = (view_x1 + view_x2)/2, view_center_y = (view_y1 + view_y2)/2;

static const auto combo_flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,            DIF_NONE),

    // global settings
    ctrl(DI_CHECKBOX,    view_x1, view_y1, view_x1 + 15, 1,         DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1, view_y1+1, view_x1 + 15, 1,       DIF_LEFTTEXT),
    
    // api settings
    ctrl(DI_TEXT,        -1, api_box_y, box_x2, box_y2,             DIF_SEPARATOR),
    ctrl(DI_TEXT,        view_x1, api_box_y+1, view_x1+15, 1,       DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, api_box_y+1, view_x2, 1,       DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1, api_box_y+2, view_x1+15, 1,       DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, api_box_y+2, view_x2, 1,       DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_x1, api_box_y+3, view_x1+15, 1,       DIF_LEFTTEXT),
    ctrl(DI_EDIT,        view_x1+15, api_box_y+3, view_x2, 1,       DIF_LEFTTEXT),
    
    // ui block
    ctrl(DI_TEXT,        -1, ui_box_y, box_x2, box_y2,                      DIF_SEPARATOR),
    ctrl(DI_CHECKBOX,    view_x1, ui_box_y+1, view_x1 + 15, 1,              DIF_LEFTTEXT),
    ctrl(DI_TEXT,        view_center_x, ui_box_y+1, view_center_x+10, 1,    DIF_LEFTTEXT),
    ctrl(DI_COMBOBOX,    view_center_x+13, ui_box_y+1, view_center_x+25, 1, combo_flags),

    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,     DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,   DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,   DIF_CENTERGROUP),
};

general_dialog::general_dialog():
    modal_dialog(&guids::ConfigGeneralDialogGuid, width, height, dlg_items_layout, L"ConfigGeneral")
{
    // general
    dialogs::set_checked(hdlg, add_to_disk_checkbox, config::is_added_to_disk_menu());
    dialogs::set_checked(hdlg, verbose_logging_checkbox, config::is_verbose_logging_enabled());

    // spotify
    dialogs::set_text(hdlg, api_client_id_edit, config::get_client_id());
    dialogs::set_text(hdlg, api_client_secret_edit, config::get_client_secret());
    dialogs::set_text(hdlg, api_port_edit, std::to_string(config::get_localhost_port()));
    
    // notifications
    dialogs::set_checked(hdlg, track_changed_notifications, config::is_track_changed_notification_enabled());
}

void general_dialog::init()
{
    static const std::vector<wstring> items{ get_text(MCfgImageShapeSquare), get_text(MCfgImageShapeCircle) };

    // image shape combo box initialization
    auto is_circled = config::is_notification_image_circled();
    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        dialogs::add_list_item(hdlg, image_shape_combo, item, (int)idx,
            (void*)item.c_str(), item.size(), idx == size_t(is_circled));
    }

    // labels
    dialogs::set_text(hdlg, dialog_box, get_text(MCfgGeneralBoxTitle));
    dialogs::set_text(hdlg, add_to_disk_checkbox, get_text(MCfgAddToDisksMenu));
    dialogs::set_text(hdlg, verbose_logging_checkbox, get_text(MCfgVerboseLog));
    dialogs::set_text(hdlg, spotify_api_separator, get_text(MCfgSpotifyTitle));
    dialogs::set_text(hdlg, api_client_id_label, get_text(MCfgSpotifyClientID));
    dialogs::set_text(hdlg, api_client_secret_label, get_text(MCfgSpotifyClientSecret));
    dialogs::set_text(hdlg, api_port_label, get_text(MCfgLocalhostServicePort));
    dialogs::set_text(hdlg, api_port_label, get_text(MCfgLocalhostServicePort));
    dialogs::set_text(hdlg, ui_separator, get_text(MCfgNotificationsTitle));
    dialogs::set_text(hdlg, track_changed_notifications, get_text(MCfgTrackChanged));
    dialogs::set_text(hdlg, image_shape_label, get_text(MCfgImageShape));
    dialogs::set_text(hdlg, ok_button, get_text(MOk));
    dialogs::set_text(hdlg, cancel_button, get_text(MCancel));
}

intptr_t general_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_button)
    {
        {
            auto ctx = config::lock_settings();
            auto &s = ctx->get_settings();

            // general
            s.add_to_disk_menu = dialogs::is_checked(hdlg, add_to_disk_checkbox);
            s.verbose_logging = dialogs::is_checked(hdlg, verbose_logging_checkbox);

            // spotify
            s.spotify_client_id = dialogs::get_text(hdlg, api_client_id_edit);
            s.spotify_client_secret = dialogs::get_text(hdlg, api_client_secret_edit);
            s.localhost_service_port = std::stoi(dialogs::get_text(hdlg, api_port_edit));

            // notifications
            s.track_changed_notification_enabled = dialogs::is_checked(hdlg, track_changed_notifications);
            s.is_circled_notification_image = dialogs::get_list_current_pos(hdlg, image_shape_combo) == 1;

            ctx->fire_events(); // notify all the listeners
        }
        config::write();
    }
    return FALSE;
}

} // namespace settings
} // namespace ui
} // namespace spotifar
