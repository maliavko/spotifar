#include "search.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "spotifar.hpp"
#include "plugin.h"
#include "spotify/interfaces.hpp"
#include "spotify/requesters.hpp"
#include "search_results.hpp"

namespace spotifar { namespace ui {

using namespace utils::far3;
namespace fs = std::filesystem;

enum controls : int
{
    no_control = -1,
    dialog_box,

    query_ip,

    types_sep,
    album_type_cb,
    artist_type_cb,
    track_type_cb,
    playlist_type_cb,

    filters_sep,
    album_filter_lbl,
    album_filter_ip,
    artist_filter_lbl,
    artist_filter_ip,
    track_filter_lbl,
    track_filter_ip,
    year_filter_lbl,
    year_filter_ip,
    upc_filter_lbl,
    upc_filter_ip,
    isrc_filter_lbl,
    isrc_filter_ip,
    genre_filter_lbl,
    genre_filter_ip,
    fresh_filter_cb,
    low_rated_filter_cb,

    buttons_sep,
    ok_btn,
    cancel_btn,
};

static const int
    query_box_y = 2,
    types_box_y = query_box_y + 1,
    filters_box_y = types_box_y + 3,

    buttons_box_y = filters_box_y + 9,
    width = 60, height = buttons_box_y + 4,
    center_x = width / 2,

    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,  box_x1, box_y1, box_x2, box_y2,                   DIF_NONE),

    ctrl(DI_EDIT,       view_x1, view_y1, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"spotifar-search-query-ip"),

    ctrl(DI_TEXT,        -1, types_box_y, view_x2, 1,                      DIF_SEPARATOR),
    ctrl(DI_CHECKBOX,   center_x-14, types_box_y+1, center_x, 1,                       DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x+2, types_box_y+1, view_x2, 1,                       DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x-14, types_box_y+2, center_x, 1,                       DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x+2, types_box_y+2, view_x2, 1,                       DIF_NONE),
    
    ctrl(DI_TEXT,       -1, filters_box_y, view_x2, 1,                      DIF_SEPARATOR),
    ctrl(DI_TEXT,       view_x1, filters_box_y+1, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+1, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"album_filter_ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+2, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+2, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"artist_filter_ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+3, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+3, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"track_filter_ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+4, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+4, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"year_filter_ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+5, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+5, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"genre_filter_ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+6, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+6, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"upc_filter_ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+7, 14, 1,                    DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       16, filters_box_y+7, view_x2, 1,                    DIF_HISTORY | DIF_USELASTHISTORY, L"", L"isrc_filter_ip"),
    ctrl(DI_CHECKBOX,   center_x-14, filters_box_y+8, center_x, 1,                       DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x+2, filters_box_y+8, view_x2, 1,                       DIF_NONE),
    
    
    // buttons block
    ctrl(DI_TEXT,       -1, buttons_box_y, view_x2, box_y2,            DIF_SEPARATOR),
    ctrl(DI_BUTTON,     view_x1, buttons_box_y+1, view_x2, box_y2,          DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,     view_x1, buttons_box_y+1, view_x2, box_y2,          DIF_CENTERGROUP),
};


search_dialog::search_dialog():
    modal_dialog(&ConfigCachesDialogGuid, width, height, dlg_items_layout, L"SearchDialog")
{
}

search_dialog::~search_dialog()
{
}

void search_dialog::init()
{
    dialogs::set_text(hdlg, dialog_box, L"Search");

    dialogs::set_text(hdlg, types_sep, L"Types");
    dialogs::set_text(hdlg, album_type_cb, L"Albums");
    dialogs::set_checked(hdlg, album_type_cb, true);
    dialogs::set_text(hdlg, artist_type_cb, L"Artists");
    dialogs::set_checked(hdlg, artist_type_cb, true);
    dialogs::set_text(hdlg, track_type_cb, L"Tracks");
    dialogs::set_checked(hdlg, track_type_cb, true);
    dialogs::set_text(hdlg, playlist_type_cb, L"Playlists");
    dialogs::set_checked(hdlg, playlist_type_cb, true);

    dialogs::set_text(hdlg, filters_sep, L"Filters");
    dialogs::set_text(hdlg, album_filter_lbl, L"Album");
    dialogs::set_text(hdlg, album_filter_ip, L"");
    dialogs::set_text(hdlg, artist_filter_lbl, L"Artist");
    dialogs::set_text(hdlg, artist_filter_ip, L"");
    dialogs::set_text(hdlg, track_filter_lbl, L"Track");
    dialogs::set_text(hdlg, track_filter_ip, L"");
    dialogs::set_text(hdlg, year_filter_lbl, L"Year");
    dialogs::set_text(hdlg, year_filter_ip, L"");
    dialogs::set_text(hdlg, genre_filter_lbl, L"Genre");
    dialogs::set_text(hdlg, genre_filter_ip, L"");
    dialogs::set_text(hdlg, upc_filter_lbl, L"UPC");
    dialogs::set_text(hdlg, upc_filter_ip, L"");
    dialogs::set_text(hdlg, isrc_filter_lbl, L"ISRC");
    dialogs::set_text(hdlg, isrc_filter_ip, L"");
    dialogs::set_text(hdlg, fresh_filter_cb, L"Fresh release");
    dialogs::set_text(hdlg, low_rated_filter_cb, L"Low rating");

    dialogs::set_text(hdlg, ok_btn, get_text(MOk));
    dialogs::set_text(hdlg, cancel_btn, get_text(MCancel));
}

intptr_t search_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_btn)
    {
        auto plugin = get_plugin();
        auto api = plugin->get_api();

        auto q = dialogs::get_text(hdlg, query_ip);

        std::vector<string> types;
        if (dialogs::is_checked(hdlg, album_type_cb))
            types.push_back("album");
        if (dialogs::is_checked(hdlg, artist_type_cb))
            types.push_back("artist");
        if (dialogs::is_checked(hdlg, track_type_cb))
            types.push_back("track");
        if (dialogs::is_checked(hdlg, playlist_type_cb))
            types.push_back("playlist");

        auto album_filter = dialogs::get_text(hdlg, album_filter_ip);
        auto artist_filter = dialogs::get_text(hdlg, artist_filter_ip);
        auto track_filter = dialogs::get_text(hdlg, track_filter_ip);
        auto year_filter = dialogs::get_text(hdlg, year_filter_ip);
        auto genre_filter = dialogs::get_text(hdlg, genre_filter_ip);
        auto upc_filter = dialogs::get_text(hdlg, upc_filter_ip);
        auto isrc_filter = dialogs::get_text(hdlg, isrc_filter_ip);

        auto req = spotify::search_requester(
            utils::utf8_encode(q), types,
            utils::utf8_encode(album_filter),
            utils::utf8_encode(artist_filter),
            utils::utf8_encode(track_filter),
            utils::utf8_encode(year_filter),
            utils::utf8_encode(genre_filter),
            utils::utf8_encode(upc_filter),
            utils::utf8_encode(isrc_filter),
            dialogs::is_checked(hdlg, fresh_filter_cb),
            dialogs::is_checked(hdlg, low_rated_filter_cb)
        );

        if (req.execute(api))
        {
            auto r = search_results_dialog(req).run();
        }

        return TRUE;
    }
    return FALSE;
}

} // namespace ui
} // namespace spotifar