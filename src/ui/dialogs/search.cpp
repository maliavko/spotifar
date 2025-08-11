#include "search.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "spotifar.hpp"
#include "plugin.h"
#include "spotify/interfaces.hpp"
#include "spotify/requesters.hpp"
#include "search_results.hpp"

namespace spotifar { namespace ui {

using no_redraw_search = no_redraw<search_dialog>;
using namespace utils::far3;

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

    genre_filter_ip,
    genre_filter_lbl,
    year_filter_ip,
    year_filter_lbl,
    fresh_filter_cb,
    low_rated_filter_cb,

    buttons_sep,
    ok_btn,
    cancel_btn,
};

static const int
    query_box_y = 2,
    filters_box_y = query_box_y + 1,
    buttons_box_y = filters_box_y + 5,
    width = 65, height = buttons_box_y + 4,

    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1,
    center_x = width / 2, quarter_w = (view_x2 - view_x1) / 4;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,  box_x1, box_y1, box_x2, box_y2,                     DIF_NONE),

    ctrl(DI_EDIT,       view_x1, view_y1, view_x2, 1,                       DIF_HISTORY, L"", L"spotifar-search-query-ip"),

    ctrl(DI_TEXT,       -1, filters_box_y, view_x2, 1,                      DIF_SEPARATOR),
    ctrl(DI_CHECKBOX,   view_x1, filters_box_y+1, center_x, 1,              DIF_NONE),
    ctrl(DI_CHECKBOX,   view_x1+quarter_w, filters_box_y+1, center_x, 1,    DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x, filters_box_y+1, center_x, 1,             DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x+quarter_w, filters_box_y+1, center_x, 1,   DIF_NONE),
    
    ctrl(DI_EDIT,       view_x1+8, filters_box_y+3, center_x+5, 1,          DIF_HISTORY, L"", L"genre-filter-ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+3, view_x1+6, 1,             DIF_RIGHTTEXT),
    ctrl(DI_EDIT,       view_x1+8, filters_box_y+4, center_x+5, 1,          DIF_HISTORY, L"", L"year-filter-ip"),
    ctrl(DI_TEXT,       view_x1, filters_box_y+4, view_x1+6, 1,             DIF_RIGHTTEXT),
    ctrl(DI_CHECKBOX,   center_x+8, filters_box_y+3, view_x2, 1,            DIF_NONE),
    ctrl(DI_CHECKBOX,   center_x+8, filters_box_y+4, view_x2, 1,            DIF_NONE),
    
    // buttons block
    ctrl(DI_TEXT,       -1, buttons_box_y, view_x2, box_y2,                 DIF_SEPARATOR),
    ctrl(DI_BUTTON,     view_x1, buttons_box_y+1, view_x2, box_y2,          DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,     view_x1, buttons_box_y+1, view_x2, box_y2,          DIF_CENTERGROUP),
};


search_dialog::search_dialog():
    modal_dialog(&guids::SearchDialogGuid, width, height, dlg_items_layout, L"SearchDialog")
{
}

void search_dialog::init()
{
    no_redraw_search nr(hdlg);
    
    {
        auto ctx = config::lock_settings();
        auto &s = ctx->get_settings();

        settings = &s.search_dialog;
    }

    dialogs::set_text(hdlg, dialog_box, get_text(MSearchTitle));

    dialogs::set_text(hdlg, album_type_cb, get_text(MSearchAlbums));
    dialogs::set_checked(hdlg, album_type_cb, settings->is_albums);
    dialogs::set_text(hdlg, artist_type_cb, get_text(MSearchArtists));
    dialogs::set_checked(hdlg, artist_type_cb, settings->is_artists);
    dialogs::set_text(hdlg, track_type_cb, get_text(MSearchTracks));
    dialogs::set_checked(hdlg, track_type_cb, settings->is_tracks);
    dialogs::set_text(hdlg, playlist_type_cb, get_text(MSearchPlaylists));
    dialogs::set_checked(hdlg, playlist_type_cb, settings->is_playlists);

    dialogs::set_text(hdlg, year_filter_lbl, get_text(MSearchYear));
    dialogs::set_text(hdlg, year_filter_ip, L"");
    dialogs::set_text(hdlg, genre_filter_lbl, get_text(MSearchGenre));
    dialogs::set_text(hdlg, genre_filter_ip, L"");
    dialogs::set_text(hdlg, fresh_filter_cb, get_text(MSearchFreshRelease));
    dialogs::set_text(hdlg, low_rated_filter_cb, get_text(MSearchLowRating));

    dialogs::set_text(hdlg, ok_btn, get_text(MOk));
    dialogs::set_text(hdlg, cancel_btn, get_text(MCancel));
}

intptr_t search_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_btn)
    {
        auto plugin = get_plugin();
        auto api = plugin->get_api();

        std::vector<string> types;
        if (dialogs::is_checked(hdlg, album_type_cb))
            types.push_back("album");
        if (dialogs::is_checked(hdlg, artist_type_cb))
            types.push_back("artist");
        if (dialogs::is_checked(hdlg, track_type_cb))
            types.push_back("track");
        if (dialogs::is_checked(hdlg, playlist_type_cb))
            types.push_back("playlist");

        auto query = dialogs::get_text(hdlg, query_ip);
        auto year_filter = dialogs::get_text(hdlg, year_filter_ip);
        auto genre_filter = dialogs::get_text(hdlg, genre_filter_ip);

        auto req = spotify::search_requester(
            utils::utf8_encode(query), {
                .types = types,
                .year = utils::utf8_encode(year_filter),
                .genre = utils::utf8_encode(genre_filter),
                .is_fresh = dialogs::is_checked(hdlg, fresh_filter_cb),
                .is_low = dialogs::is_checked(hdlg, low_rated_filter_cb)
            });

        if (req.execute(api))
        {
            search_results_dialog(req).run();
        }

        return TRUE;
    }
    return FALSE;
}

bool search_dialog::handle_btn_clicked(int ctrl_id, std::uintptr_t param)
{
    switch (ctrl_id)
    {
        case album_type_cb:
            settings->is_albums = (bool)param;
            return true;
        case artist_type_cb:
            settings->is_artists = (bool)param;
            return true;
        case track_type_cb:
            settings->is_tracks = (bool)param;
            return true;
        case playlist_type_cb:
            settings->is_playlists = (bool)param;
            return true;
    }

    return false;
}

} // namespace ui
} // namespace spotifar