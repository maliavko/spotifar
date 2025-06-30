#include "search_results.hpp"
#include "search.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "spotifar.hpp"
#include "plugin.h"
#include "spotify/interfaces.hpp"

namespace spotifar { namespace ui {

using no_redraw_search = no_redraw<search_results_dialog>;
using namespace utils::far3;

enum controls : int
{
    no_control = -1,
    dialog_box,

    results_list,

    buttons_sep,
    new_search_btn,
    ok_btn,
    cancel_btn,
};

static const int
    width = 105, height = 35,
    center_x = width / 2,

    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,  box_x1, box_y1, box_x2, box_y2,         DIF_NONE),

    // listbox
    ctrl(DI_LISTBOX,    view_x1, view_y1, view_x2, view_y2-2,   DIF_LISTNOBOX),
    
    // buttons block
    ctrl(DI_TEXT,       -1, view_y2-1, view_x2, 1,              DIF_SEPARATOR2),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,           DIF_CENTERGROUP),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,           DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,           DIF_CENTERGROUP),
};

static void show_artist_page(api_weak_ptr_t api_proxy, const spotify::data_item_t &item)
{
    if (const auto &artist = static_cast<const spotify::artist_t&>(item))
        events::show_artist(api_proxy, artist, multiview_builder_t::tracks_idx);
}

static void start_artist_playback(api_weak_ptr_t api_proxy, const spotify::data_item_t &item)
{
    if (const auto &artist = static_cast<const spotify::artist_t&>(item))
    {
        if (auto api = api_proxy.lock())
            api->start_playback(artist.get_uri());
    }
}

static void show_album_page(api_weak_ptr_t api_proxy, const spotify::data_item_t &item)
{
    if (const auto &album = static_cast<const spotify::simplified_album_t&>(item))
    {
        if (auto api = api_proxy.lock())
        {
            if (const auto &artist = api->get_artist(album.get_artist().id))
            {
                events::show_artist(api, artist, multiview_builder_t::albums_idx);
                events::select_item(album.id);
            }
        }
    }
}

static void start_album_playback(api_weak_ptr_t api_proxy, const spotify::data_item_t &item)
{
    if (const auto &album = static_cast<const spotify::simplified_album_t&>(item))
    {
        if (auto api = api_proxy.lock())
            api->start_playback(album.get_uri());
    }
}

static void show_track_page(api_weak_ptr_t api_proxy, const spotify::data_item_t &item)
{
    if (const auto &track = static_cast<const spotify::track_t&>(item))
    {
        events::show_album_tracks(api_proxy, track.album);
        events::select_item(track.id);
    }
}

static void start_track_playback(api_weak_ptr_t api_proxy, const spotify::data_item_t &item)
{
    if (const auto &track = static_cast<const spotify::track_t&>(item))
    {
        if (auto api = api_proxy.lock())
            api->start_playback(std::vector{ track.get_uri() });
    }
}

template<typename... Args>
static inline wstring format(const wstring &fmt, Args&&... args)
{
    return std::vformat(fmt, std::make_wformat_args(args...));
}

static wstring format_followers(uintmax_t followers)
{
    return utils::to_wstring(utils::format_number(followers, 1000, " KMGTPE", 100.));
}

//-----------------------------------------------------------------------------------------------
search_results_dialog::search_results_dialog(const spotify::search_requester &r):
    modal_dialog(&SearchResultsDialogGuid, width, height, dlg_items_layout, L"SearchResultsDialog"),
    requester(r)
{
    rebuild_items();
    utils::events::start_listening<collection_observer>(this);
}

search_results_dialog::~search_results_dialog()
{
    utils::events::stop_listening<collection_observer>(this);
    items.clear();
}

void search_results_dialog::rebuild_items()
{
    items.clear();

    auto plugin = get_plugin();
    auto api = plugin->get_api();

    // artists block
    if (requester.artists.size() > 0)
    {
        static const wstring
            artist_tpl = L"{:37}│{:30}│{:3}│{: >10}│{: ^7}",
            artists_title = format(
                artist_tpl,
                get_text(MSortColName),
                get_text(MSortColGenre),
                get_text(MSortColSaved),
                get_text(MSortColFollow),
                get_text(MSortColPopularity));

        items.push_back({ get_text(MSearchArtists), LIF_SEPARATOR });
        items.push_back({ artists_title, LIF_DISABLE });
        items.push_back({ L"", LIF_SEPARATOR });
        for (const auto &artist: requester.artists)
        {
            bool is_saved = api->get_library()->is_artist_followed(artist.id);

            wstring label = format(
                artist_tpl,
                utils::trunc(artist.name, 37),
                utils::trunc(artist.get_main_genre(), 30),
                is_saved ? L" + " : L"",
                format_followers(artist.followers_total),
                artist.popularity
            );

            items.push_back({
                label, LIF_NONE, &artist, show_artist_page, start_artist_playback
            });
        }
    }

    // albums block
    if (requester.albums.size() > 0)
    {
        static const wstring
            albums_tpl = L"{: ^6}│{:40}│{:27}│{:3}│{: >4}│{: ^6}",
            albums_title = format(
                albums_tpl,
                get_text(MSortColYear),
                get_text(MSortColName),
                get_text(MSortColArtist),
                get_text(MSortColSaved),
                get_text(MSortColTracksCount),
                get_text(MSortColType));

        items.push_back({ L"", LIF_DISABLE });
        items.push_back({ get_text(MSearchAlbums), LIF_SEPARATOR });
        items.push_back({ albums_title, LIF_DISABLE });
        items.push_back({ L"", LIF_SEPARATOR });
        for (const auto &album: requester.albums)
        {
            bool is_saved = api->get_library()->is_album_saved(album.id);

            wstring label = format(
                albums_tpl,
                utils::to_wstring(album.get_release_year()),
                utils::trunc(album.name, 40),
                utils::trunc(album.get_artist().name, 27),
                is_saved ? L" + " : L"",
                album.total_tracks,
                album.get_type_abbrev()
            );

            items.push_back({
                label, LIF_NONE, &album, show_album_page, start_album_playback
            });
        }
    }

    // tracks block
    if (requester.tracks.size() > 0)
    {
        static const wstring
            tracks_tpl = L"{: ^6}│{:25}│{:20}│{:20}│{:3}│{:3}│{: ^7}",
            tracks_title = format(
                tracks_tpl,
                get_text(MSortColYear),
                get_text(MSortColName),
                get_text(MSortColArtist),
                get_text(MSortColAlbum),
                get_text(MSortColSaved),
                get_text(MSortColExplicit),
                get_text(MSortColDuration));
                
        items.push_back({ L"", LIF_DISABLE });
        items.push_back({ get_text(MSearchTracks), LIF_SEPARATOR });
        items.push_back({ tracks_title, LIF_DISABLE });
        items.push_back({ L"", LIF_SEPARATOR });
        for (const auto &track: requester.tracks)
        {
            bool is_saved = api->get_library()->is_track_saved(track.id);
            
            auto duration = std::chrono::milliseconds(track.duration_ms);
            wstring track_length;
            if (duration < 1h)
                track_length = std::format(L"{:%M:%S}", duration);
            else
                track_length = std::format(L"{:%Hh%M}", duration);

            wstring label = format(
                tracks_tpl,
                utils::to_wstring(track.album.get_release_year()),
                utils::trunc(track.name, 25),
                utils::trunc(track.get_artist().name, 20),
                utils::trunc(track.album.name, 20),
                is_saved ? L" + " : L"",
                track.is_explicit ? L" * " : L"",
                track_length.substr(0, 5)
            );

            items.push_back({
                label, LIF_NONE, &track, show_track_page, start_track_playback
            });
        }
    }
}

void search_results_dialog::refresh_list(const std::unordered_set<spotify::item_id_t> &ids)
{
    no_redraw_search nr(hdlg);

    // saving index of the item under cursor to recover it later
    auto cur_pos = dialogs::get_list_current_pos(hdlg, results_list);

    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        if (item.data && ids.contains(item.data->id))
            dialogs::update_list_item(hdlg, results_list, item.label, (int)idx,
                (void*)&item, sizeof(item), idx == cur_pos, item.flags);
    }
}

void search_results_dialog::init()
{
    no_redraw_search nr(hdlg);
    
    dialogs::set_text(hdlg, dialog_box, get_text(MSearchResultsTitle));
    
    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        dialogs::add_list_item(hdlg, results_list, item.label, (int)idx,
            (void*)&item, sizeof(item), false, item.flags);
    }

    dialogs::set_text(hdlg, new_search_btn, get_text(MSearchResultsNewBtn));
    dialogs::set_text(hdlg, ok_btn, get_text(MOk));
    dialogs::set_text(hdlg, cancel_btn, get_text(MCancel));
}

intptr_t search_results_dialog::handle_result(intptr_t dialog_run_result)
{
    if (dialog_run_result == ok_btn)
    {
        auto plugin = get_plugin();
        auto api = plugin->get_api();

        if (auto item = dialogs::get_list_current_item_data<const item_entry*>(hdlg, results_list))
            item->show_handler(api->get_ptr(), *item->data);

        return TRUE;
    }
    else if (dialog_run_result == new_search_btn)
    {
        search_dialog().run();
        return TRUE;
    }
    return FALSE;
}

bool search_results_dialog::handle_key_pressed(int ctrl_id, int combined_key)
{
    if (ctrl_id == results_list && combined_key == VK_F4)
    {
        auto plugin = get_plugin();
        auto api = plugin->get_api();

        if (auto item = dialogs::get_list_current_item_data<const item_entry*>(hdlg, results_list))
            item->play_handler(api->get_ptr(), *item->data);

        return true;
    }
    return false;
}

void search_results_dialog::on_tracks_statuses_received(const spotify::item_ids_t &ids)
{
    std::unordered_set<spotify::item_id_t> unique_ids(ids.begin(), ids.end());

    const auto &it = std::find_if(requester.tracks.begin(), requester.tracks.end(),
        [&unique_ids](auto &track) { return unique_ids.contains(track.id); });

    // if any of view's tracks are changed, we need to refresh the panel
    if (it != requester.tracks.end())
    {
        rebuild_items();
        refresh_list(unique_ids);
    }
}

void search_results_dialog::on_artists_statuses_received(const spotify::item_ids_t &ids)
{
    std::unordered_set<spotify::item_id_t> unique_ids(ids.begin(), ids.end());

    const auto &it = std::find_if(requester.artists.begin(), requester.artists.end(),
        [&unique_ids](auto &artist) { return unique_ids.contains(artist.id); });

    // if any of view's artists are changed, we need to refresh the panel
    if (it != requester.artists.end())
    {
        rebuild_items();
        refresh_list(unique_ids);
    }
}

void search_results_dialog::on_albums_statuses_received(const spotify::item_ids_t &ids)
{
    std::unordered_set<spotify::item_id_t> unique_ids(ids.begin(), ids.end());

    const auto &it = std::find_if(requester.albums.begin(), requester.albums.end(),
        [&unique_ids](auto &album) { return unique_ids.contains(album.id); });

    // if any of view's albums are changed, we need to refresh the panel
    if (it != requester.albums.end())
    {
        rebuild_items();
        refresh_list(unique_ids);
    }
}

} // namespace ui
} // namespace spotifar