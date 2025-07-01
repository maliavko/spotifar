#include "albums.hpp"
#include "lng.hpp"
#include "config.hpp"

namespace spotifar { namespace ui { namespace filters {

using namespace utils::far3;

enum controls: int
{
    dialog_box,

    albums_cb,
    singles_cb,
    appears_on_cb,
    compilations_cb,
    
    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    width = 30, height = 12,
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const auto combo_flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,            DIF_NONE),

    ctrl(DI_CHECKBOX,    view_x1+1, view_y1+1, view_x1 + 15, 1,     DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+1, view_y1+2, view_x1 + 15, 1,     DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+1, view_y1+3, view_x1 + 15, 1,     DIF_LEFTTEXT),
    ctrl(DI_CHECKBOX,    view_x1+1, view_y1+4, view_x1 + 15, 1,     DIF_LEFTTEXT),

    // buttons block
    ctrl(DI_TEXT,        box_x1, box_y2-2, box_x2, 1,               DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, box_y2-1, box_x2, 1,               DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,      box_x1, box_y2-1, box_x2, 1,               DIF_CENTERGROUP),
};

albums_filters_dialog::albums_filters_dialog():
    modal_dialog(&FilterDialogGuid, width, height, dlg_items_layout, L"AlbumsFiltersDialog")
{
}

void albums_filters_dialog::init()
{
    dialogs::set_text(hdlg, dialog_box, get_text(MFiltersAlbumsTitle));

    dialogs::set_text(hdlg, albums_cb, get_text(MFiltersAlbums));
    dialogs::set_text(hdlg, singles_cb, get_text(MFiltersSingles));
    dialogs::set_text(hdlg, appears_on_cb, get_text(MFiltersAppearsOn));
    dialogs::set_text(hdlg, compilations_cb, get_text(MFiltersCompilations));

    {
        auto ctx = config::lock_settings();
        auto &s = ctx->get_settings();
        
        dialogs::set_checked(hdlg, albums_cb, s.filters.albums_lps);
        dialogs::set_checked(hdlg, singles_cb, s.filters.albums_eps);
        dialogs::set_checked(hdlg, appears_on_cb, s.filters.albums_appears_on);
        dialogs::set_checked(hdlg, compilations_cb, s.filters.albums_compilations);
    }

    dialogs::set_text(hdlg, ok_button, get_text(MOk));
    dialogs::set_text(hdlg, cancel_button, get_text(MCancel));
}

intptr_t albums_filters_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_button)
    {
        {
            auto ctx = config::lock_settings();
            auto &s = ctx->get_settings();

            s.filters.albums_lps = dialogs::is_checked(hdlg, albums_cb);
            s.filters.albums_eps = dialogs::is_checked(hdlg, singles_cb);
            s.filters.albums_appears_on = dialogs::is_checked(hdlg, appears_on_cb);
            s.filters.albums_compilations = dialogs::is_checked(hdlg, compilations_cb);

            ctx->fire_events(); // notify all the listeners
        }
        config::write();
    }
    return FALSE;
}

} // namespace filter
} // namespace ui
} // namespace spotifar