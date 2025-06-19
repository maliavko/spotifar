#include "search_results.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "spotifar.hpp"
#include "plugin.h"
#include "spotify/interfaces.hpp"

namespace spotifar { namespace ui {

using namespace utils::far3;
namespace fs = std::filesystem;

enum controls : int
{
    no_control = -1,
    dialog_box,

    results_list,

    buttons_sep,
    ok_btn,
    cancel_btn,
};

static const int
    width = 120, height = 35,
    center_x = width / 2,

    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,  box_x1, box_y1, box_x2, box_y2,                   DIF_NONE),

    ctrl(DI_LISTBOX,       view_x1, view_y1, view_x2, view_y2-2,        DIF_LISTNOBOX),
    
    // buttons block
    ctrl(DI_TEXT,       -1, view_y2-1, view_x2, 1,            DIF_SEPARATOR2),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,          DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,          DIF_CENTERGROUP),
};


search_results_dialog::search_results_dialog(const spotify::search_requester &r):
    modal_dialog(&ConfigHotkeysDialogGuid, width, height, dlg_items_layout, L"SearchResultsDialog"),
    requester(r)
{
}

search_results_dialog::~search_results_dialog()
{
}

void search_results_dialog::init()
{
    dialogs::set_text(hdlg, dialog_box, L"Search Results");
    
    size_t idx = 0;
    
    dialogs::add_list_item(hdlg, results_list, L"Artists", (int)idx++, nullptr, 0, false, LIF_SEPARATOR);

    for (size_t i = 0; i < requester.artists.size(); i++)
    {
        const auto &item = requester.artists[i];
        dialogs::add_list_item(hdlg, results_list, item.name, (int)idx++, (void*)&item, sizeof(item));
    }
    
    dialogs::add_list_item(hdlg, results_list, L"Albums", (int)idx++, nullptr, 0, false, LIF_SEPARATOR);
    
    for (size_t i = 0; i < requester.albums.size(); i++)
    {
        const auto &item = requester.albums[i];
        dialogs::add_list_item(hdlg, results_list, item.name, (int)idx++, (void*)&item, sizeof(item));
    }
    
    dialogs::add_list_item(hdlg, results_list, L"Tracks", (int)idx++, nullptr, 0, false, LIF_SEPARATOR);
    
    for (size_t i = 0; i < requester.tracks.size(); i++)
    {
        const auto &item = requester.tracks[i];
        dialogs::add_list_item(hdlg, results_list, item.name, (int)idx++, (void*)&item, sizeof(item));
    }

    dialogs::set_text(hdlg, ok_btn, get_text(MOk));
    dialogs::set_text(hdlg, cancel_btn, get_text(MCancel));
}


intptr_t search_results_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_btn)
    {
        auto plugin = get_plugin();
        auto api = plugin->get_api();

        auto item = dialogs::get_list_current_item_data<spotify::track_t*>(hdlg, results_list);

        return TRUE;
    }
    return FALSE;
}

} // namespace ui
} // namespace spotifar