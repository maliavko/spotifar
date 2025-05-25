#include "tracks.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
tracks_base_view::tracks_base_view(HANDLE panel, api_proxy_ptr api, const wstring &title, return_callback_t callback):
    view_abstract(panel, title, callback),
    api_proxy(api)
{
}

const view_abstract::sort_modes_t& tracks_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Track Number",  SM_EXT,     VK_F4 + mods::ctrl },
        { L"Duration",      SM_SIZE,    VK_F5 + mods::ctrl },
    };
    return modes;
}

const view_abstract::items_t& tracks_base_view::get_items()
{
    static view_abstract::items_t items; items.clear();

    for (const auto &track: get_tracks())
    {
        std::vector<wstring> columns;
        
        // column C0 - is explicit lyrics
        columns.push_back(track.is_explicit ? L" * " : L"");

        // column C1 - duration
        auto duration = std::chrono::milliseconds(track.duration_ms);
        wstring track_length;
        if (duration < 1h)
            track_length = std::format(L"{:%M:%S}", duration);
        else
            track_length = std::format(L"{:%Hh%M}", duration);
        columns.push_back(track_length.substr(0, 5));
        
        // column C2 - track number
        // eaither just 01,02 or in a multisdics case - 1/02, 1/02 etc.
        wstring track_number = std::format(L"{:02}", track.track_number);
        columns.push_back(std::format(L"{: ^7}", track_number));

        // list of artists is used as a description field
        std::vector<wstring> artists_names;
        std::transform(track.artists.cbegin(), track.artists.cend(), back_inserter(artists_names),
            [](const auto &a) { return a.name; });
        
        bool is_item_selected = false;
        if (auto api = api_proxy.lock())
        {
            const auto &pstate = api->get_playback_state();
            is_item_selected = pstate.item.is_valid() && pstate.item.id == track.id;
        }

        items.push_back({
            track.id,
            track.name,
            utils::string_join(artists_names, L", "),
            FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<simplified_track_t*>(&track),
            is_item_selected
        });
    }

    return items;
}

void tracks_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    // TODO: if there is no multidics, the first column can be shorter?
    // or make it the same to Spotify - no disc number, just sorting
    static const wchar_t* titles_3[] = { L"##", L"Name", L"[E]", L"Time" };
    modes[3].ColumnTypes = L"C2,NON,C0,C1";
    modes[3].ColumnWidths = L"7,0,3,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

intptr_t tracks_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const simplified_track_t*>(data1),
        &item2 = static_cast<const simplified_track_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_EXT:
            if (item1->disc_number == item2->disc_number)
            {
                if (item1->track_number == item2->track_number)
                    return 0;
                return item1->track_number < item2->track_number ? -1 : 1;
            }
            return item1->disc_number < item2->disc_number ? -1 : 1;

        case SM_SIZE:
            if (item1->duration_ms == item2->duration_ms)
                return 0;
            return item1->duration_ms < item2->duration_ms ? -1 : 1;
    }
    return -2;
}

intptr_t tracks_base_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_RETURN + utils::keys::mods::shift:
        {
            auto item = panels::get_current_item(get_panel_handle());
            if (item != nullptr)
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    log::global->info("Starting playback from the tracks view, {}", user_data->id);
                    // if (start_playback(user_data->id))
                    // {
                    //     events::show_player();
                    //     return TRUE;
                    // }
                    start_playback(user_data->id);
                    return TRUE;
                }
            }
            else
                log::global->error("There is an error occured while getting a current panel item");

            return TRUE;
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------------------------------------
album_tracks_view::album_tracks_view(HANDLE panel, api_proxy_ptr api_proxy, const album_t &album,
                                     return_callback_t callback):
    tracks_base_view(panel, api_proxy, album.name, callback),
    album(album)
{
    utils::events::start_listening<playback_observer>(this);
    
    if (auto api = api_proxy.lock())
        collection = api->get_album_tracks(album.id);
}

album_tracks_view::~album_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t album_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

bool album_tracks_view::start_playback(const string &track_id)
{
    if (auto api = api_proxy.lock())
    {
        api->start_playback(album.get_uri(), track_t::make_uri(track_id));
        return true;
    }
    return false;
}

std::generator<const simplified_track_t&> album_tracks_view::get_tracks()
{
    if (collection->fetch())
        for (const auto &t: *collection)
            co_yield t;
}

void album_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    if (album.id == track.album.id) // the currently playing track is from this album
    {
        events::refresh_panel(get_panel_handle());

        // experimental code to select the currently playing item on the panel
        // panels::clear_selection(get_panel_handle());

        // if (auto track_idx = get_item_idx(track.id))
        //     panels::select_item(get_panel_handle(), track_idx);
    }
}

//-----------------------------------------------------------------------------------------------------------
recent_tracks_view::recent_tracks_view(HANDLE panel, api_proxy_ptr api):
    tracks_base_view(panel, api, get_text(MPanelTracksItemLabel),
                     std::bind(events::show_root, api))
{
    rebuild_items();

    utils::events::start_listening<play_history_observer>(this);
}

recent_tracks_view::~recent_tracks_view()
{
    items.clear();

    utils::events::stop_listening<play_history_observer>(this);
}

config::settings::view_t recent_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

const view_abstract::sort_modes_t& recent_tracks_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Played", SM_MTIME, VK_F6 + mods::ctrl });
    }
    return modes;
}

intptr_t recent_tracks_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_track_t*>(data1),
            &item2 = static_cast<const history_track_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
}

void recent_tracks_view::rebuild_items()
{
    items.clear();

    if (auto api = api_proxy.lock())
        for (const auto &item: api->get_play_history())
            items.push_back(history_track_t{ {item.track}, item.played_at });
}

bool recent_tracks_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const simplified_track_t&> recent_tracks_view::get_tracks()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_tracks_view::on_items_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}

//-----------------------------------------------------------------------------------------------------------
saved_tracks_view::saved_tracks_view(HANDLE panel, api_proxy_ptr api_proxy):
    tracks_base_view(panel, api_proxy, get_text(MPanelTracksItemLabel),
                     std::bind(events::show_root, api_proxy))
{
    utils::events::start_listening<playback_observer>(this);
    if (auto api = api_proxy.lock())
        collection = api->get_saved_tracks();
}

saved_tracks_view::~saved_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t saved_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

bool saved_tracks_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const simplified_track_t&> saved_tracks_view::get_tracks()
{
    if (collection->fetch())
        for (const auto &t: *collection)
            co_yield t;
}

void saved_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
}

//-----------------------------------------------------------------------------------------------------------
playing_queue_view::playing_queue_view(HANDLE panel, api_proxy_ptr api):
    tracks_base_view(panel, api, get_text(MPanelTracksItemLabel),
                     std::bind(events::show_root, api))
{
    utils::events::start_listening<playback_observer>(this);
}

playing_queue_view::~playing_queue_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t playing_queue_view::get_default_settings() const
{
    return { 0, false, 6 };
}

bool playing_queue_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const simplified_track_t&> playing_queue_view::get_tracks()
{
    static playing_queue_t playing_queue;
    
    if (auto api = api_proxy.lock())
    {
        playing_queue = api->get_playing_queue();

        // currently playing item
        co_yield playing_queue.currently_playing;
        
        // queued items
        for (const auto &t: playing_queue.queue)
            co_yield t;
    }
}

const view_abstract::sort_modes_t& playing_queue_view::get_sort_modes() const
{
    static sort_modes_t modes = {}; // no sorting modes for the view
    return modes;
}

intptr_t playing_queue_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const saved_track_t*>(data1),
        &item2 = static_cast<const saved_track_t*>(data2);

    return item1->added_at.compare(item2->added_at);
}

void playing_queue_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    events::refresh_panel(get_panel_handle());
}

void playing_queue_view::on_shuffle_state_changed(bool shuffle_state)
{
    events::refresh_panel(get_panel_handle());
}

//-----------------------------------------------------------------------------------------------------------
recently_liked_tracks_view::recently_liked_tracks_view(HANDLE panel, api_proxy_ptr api_proxy):
    tracks_base_view(panel, api_proxy, get_text(MPanelRecentlyLikedTracksLabel),
                     std::bind(events::show_browse, api_proxy))
{
    if (auto api = api_proxy.lock())
        collection = api->get_saved_tracks();
}

config::settings::view_t recently_liked_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

bool recently_liked_tracks_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

const view_abstract::sort_modes_t& recently_liked_tracks_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, VK_F6 + mods::ctrl });
    }
    return modes;
}

intptr_t recently_liked_tracks_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const saved_track_t*>(data1),
            &item2 = static_cast<const saved_track_t*>(data2);

        return item1->added_at.compare(item2->added_at);
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
}

std::generator<const simplified_track_t&> recently_liked_tracks_view::get_tracks()
{
    // requesting only three pages of the data
    if (collection->fetch(false, true, 3))
        for (const auto &t: *collection)
            co_yield t;
}

} // namespace ui
} // namespace spotifar