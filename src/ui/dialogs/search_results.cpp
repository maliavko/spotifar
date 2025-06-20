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
    width = 105, height = 35,
    center_x = width / 2,

    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,  box_x1, box_y1, box_x2, box_y2,             DIF_NONE),

    ctrl(DI_LISTBOX,       view_x1, view_y1, view_x2, view_y2-2,    DIF_LISTNOBOX),
    
    // buttons block
    ctrl(DI_TEXT,       -1, view_y2-1, view_x2, 1,                  DIF_SEPARATOR2),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,               DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,     view_x1, view_y2, view_x2, 1,               DIF_CENTERGROUP),
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
            api->start_playback({ track.get_uri() });
    }
}

search_results_dialog::search_results_dialog(const spotify::search_requester &r):
    modal_dialog(&ConfigHotkeysDialogGuid, width, height, dlg_items_layout, L"SearchResultsDialog"),
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
    items.push_back({ L"Artists", LIF_SEPARATOR });
    items.push_back({ std::format(L"{:67}│{:3}│{: ^10}│{: ^7}", L"Name", L"[+]", L"Followers", L"Pop %"), LIF_DISABLE });
    items.push_back({ L"", LIF_SEPARATOR });
    for (const auto &artist: requester.artists)
    {
        auto followers = artist.followers_total;
        wstring followers_lbl = L"";
        if (followers < 1000000)
            followers_lbl = std::format(L"{:9}", followers);
        else if (followers < 1000000000)
            followers_lbl = std::format(L"{:7.2f} M", followers / 1000000.0);
        else if (followers < 1000000000000)
            followers_lbl = std::format(L"{:7.2f} B", followers / 1000000000.0);

        bool is_saved = api->get_library()->is_artist_followed(artist.id);

        wstring label = std::format(
            L"{:67}│{:3}│{:10}│{:7}",
            utils::trunc(artist.name, 67), is_saved ? L" + " : L"", followers_lbl, artist.popularity
        );

        items.push_back({
            label, LIF_NONE, &artist, show_artist_page, start_artist_playback
        });
    }

    // albums block
    items.push_back({ L"", LIF_DISABLE });
    items.push_back({ L"Albums", LIF_SEPARATOR });
    items.push_back({ std::format(L"{: ^6}│{:40}│{:27}│{:3}│{: ^4}│{: ^6}", L"Year", L"Name", L"Artist", L"[+]", L"Tx", L"Type"), LIF_DISABLE });
    items.push_back({ L"", LIF_SEPARATOR });
    for (const auto &album: requester.albums)
    {
        bool is_saved = api->get_library()->is_album_saved(album.id);

        wstring label = std::format(
            L"{: ^6}│{:40}│{:27}│{:3}│{:4}│{: ^6}",
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

    // tracks block
    items.push_back({ L"", LIF_DISABLE });
    items.push_back({ L"Tracks", LIF_SEPARATOR });
    items.push_back({ std::format(L"{: ^6}│{:25}│{:20}│{:20}│{:3}│{:3}│{: ^7}", L"Year", L"Name", L"Artist", L"Album", L"[+]", L"[E]", L"Duration"), LIF_DISABLE });
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

        wstring label = std::format(
            L"{: ^6}│{:25}│{:20}│{:20}│{:3}│{:3}│{: ^7}",
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

void search_results_dialog::refresh_list(const std::unordered_set<spotify::item_id_t> &ids)
{   
    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        if (item.data && ids.contains(item.data->id))
            dialogs::update_list_item(hdlg, results_list, item.label, (int)idx,
                (void*)&item, sizeof(item), false, item.flags);
    }
}

void search_results_dialog::init()
{
    dialogs::set_text(hdlg, dialog_box, L"Search Results");
    
    for (size_t idx = 0; idx < items.size(); idx++)
    {
        const auto &item = items[idx];
        dialogs::add_list_item(hdlg, results_list, item.label, (int)idx,
            (void*)&item, sizeof(item), false, item.flags);
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

        if (auto item = dialogs::get_list_current_item_data<const item_entry*>(hdlg, results_list))
            item->show_handler(api->get_ptr(), *item->data);

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