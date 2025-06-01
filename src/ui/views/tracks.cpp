#include "tracks.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
const view_abstract::sort_modes_t& tracks_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,            { VK_F3, LEFT_CTRL_PRESSED } },
        { L"Release date",  SM_ATIME,           { VK_F4, LEFT_CTRL_PRESSED } },
        { L"Popularity",    SM_COMPRESSEDSIZE,  { VK_F5, LEFT_CTRL_PRESSED } },
        { L"Album name",    SM_OWNER,           { VK_F6, LEFT_CTRL_PRESSED } },
        { L"Artist name",   SM_CHTIME,          { VK_F7, LEFT_CTRL_PRESSED } },
        { L"Length",        SM_SIZE,            { VK_F8, LEFT_CTRL_PRESSED } },
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
        columns.push_back(std::format(L"{: ^7}", track_length.substr(0, 5)));
        
        // column C2 - album's release year
        columns.push_back(std::format(L"{: ^6}",
            utils::to_wstring(track.album.get_release_year())));

        // column C3 - main artist's name
        columns.push_back(track.get_artist().name);
        
        // column C4 - track's popularity
        columns.push_back(std::format(L"{:5}", track.popularity));

        // column C5 - album's name
        columns.push_back(track.album.name);

        // column C6 - album's type
        columns.push_back(std::format(L"{: ^6}", track.album.get_type_abbrev()));
        
        // inherited views custom columns
        const auto &extra = get_extra_columns(track);
        columns.insert(columns.end(), extra.begin(), extra.end());
        
        bool is_item_selected = false;
        if (auto api = api_proxy.lock())
        {
            const auto &pstate = api->get_playback_state();
            is_item_selected = pstate.item && pstate.item.id == track.id;
        }

        items.push_back({
            track.id,
            track.name,
            L"",
            FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<track_t*>(&track),
            is_item_selected
        });
    }
    return items;
}

void tracks_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"[E]", L"Length", L"Pop %", };
    modes[3].ColumnTypes = L"NON,C0,C1,C4";
    modes[3].ColumnWidths = L"0,3,7,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Name", L"Artist", L"Length" };
    modes[4].ColumnTypes = L"NON,C3,C1";
    modes[4].ColumnWidths = L"0,30,7";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Name", L"Artist", L"Album", L"Type", L"Yr", L"[E]", L"Length", L"Pop %", };
    modes[5].ColumnTypes = L"NON,C3,C5,C6,C2,C0,C1,C4";
    modes[5].ColumnWidths = L"0,35,35,6,6,3,7,5";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { L"Name", L"Artist", L"[E]", L"Length", L"Pop %", };
    modes[6].ColumnTypes = L"NON,C3,C0,C1,C4";
    modes[6].ColumnWidths = L"0,30,3,7,5";
    modes[6].ColumnTitles = titles_6;
    modes[6].StatusColumnTypes = NULL;
    modes[6].StatusColumnWidths = NULL;

    static const wchar_t* titles_7[] = { L"Name", L"Album", L"Yr", L"[E]", L"Length", L"Pop %", };
    modes[7].ColumnTypes = L"NON,C5,C2,C0,C1,C4";
    modes[7].ColumnWidths = L"0,30,6,3,7,5";
    modes[7].ColumnTitles = titles_7;
    modes[7].StatusColumnTypes = NULL;
    modes[7].StatusColumnWidths = NULL;

    static const wchar_t* titles_8[] = { L"Name", L"Artist", L"Album", L"Yr", };
    modes[8].ColumnTypes = L"NON,C3,C5,C2";
    modes[8].ColumnWidths = L"0,30,30,6";
    modes[8].ColumnTitles = titles_8;
    modes[8].StatusColumnTypes = NULL;
    modes[8].StatusColumnWidths = NULL;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

intptr_t tracks_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const track_t*>(data1),
        &item2 = static_cast<const track_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME: // by name
            return item1->name.compare(item2->name);

        case SM_SIZE: // by track duration
            if (item1->duration_ms != item2->duration_ms)
                return item1->duration_ms < item2->duration_ms ? -1 : 1;
            return 0;

        case SM_COMPRESSEDSIZE: // by popularity
            if (item1->popularity != item2->popularity)
                return item1->popularity < item2->popularity ? -1 : 1;
            return 0;

        case SM_OWNER: // by album name
            return item1->album.name.compare(item2->album.name);

        case SM_CHTIME: // by artist name
            return item1->get_artist().name.compare(item2->get_artist().name);

        case SM_ATIME: // by release date
            return item1->album.release_date.compare(item2->album.release_date);
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
album_tracks_view::album_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy, const simplified_album_t &album,
                                     return_callback_t callback):
    tracks_base_view(panel, api_proxy, album.name, callback),
    album(album)
{
    utils::events::start_listening<playback_observer>(this);
    rebuild_items();
}

album_tracks_view::~album_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
    album_tracks.clear();
}

void album_tracks_view::rebuild_items()
{
    album_tracks.clear();

    if (auto api = api_proxy.lock())
    {
        if (const auto &tracks = api->get_album_tracks(album.id); tracks->fetch())
        {
            item_ids_t tracks_ids;

            for (const auto &track: *tracks)
            {
                tracks_ids.push_back(track.id);

                if (!is_multidisc && track.disc_number > 1)
                    is_multidisc = true;
            }

            album_tracks = api->get_tracks(tracks_ids);
        }
    }
}

config::settings::view_t album_tracks_view::get_default_settings() const
{
    // sort mode - by `Track number`; ascending; view mode - F3
    return { 7, false, 3 };
}

void album_tracks_view::update_panel_info(OpenPanelInfo *info)
{
    tracks_base_view::update_panel_info(info);
    
    auto *modes = const_cast<PanelMode*>(info->PanelModesArray);

    // adding track number column to some of the view modes
    size_t n_width = is_multidisc ? 7 : 4;
    static const wchar_t* n_label = is_multidisc ? L"#/#" : L" #";
    static struct
    {
        std::vector<const wchar_t*> titles;
        wstring width, types;
    } columns[9];

    columns[3] =
    {
        { n_label, L"Name", L"[E]", L"Length", L"Pop %", },
        std::format(L"{},0,3,7,5", n_width),
        L"C7,NON,C0,C1,C4"
    };

    columns[4] =
    {
        { n_label, L"Name", L"Artist", L"Length", },
        std::format(L"{},0,30,7", n_width),
        L"C7,NON,C3,C1"
    };

    columns[5] =
    {
        { n_label, L"Name", L"Artist", L"Album", L"Type", L"Yr", L"[E]", L"Length", L"Pop %", },
        std::format(L"{},0,35,35,6,6,3,7,5", n_width),
        L"C7,NON,C3,C5,C6,C2,C0,C1,C4"
    };

    columns[6] =
    {
        { n_label, L"Name", L"Artist", L"[E]", L"Length", L"Pop %", },
        std::format(L"{},0,30,3,7,5", n_width),
        L"C7,NON,C3,C0,C1,C4"
    };

    columns[7] =
    {
        { n_label, L"Name", L"Album", L"Yr", L"[E]", L"Length", L"Pop %", },
        std::format(L"{},0,30,6,3,7,5", n_width),
        L"C7,NON,C5,C2,C0,C1,C4"
    };

    columns[8] =
    {
        { n_label, L"Name", L"Artist", L"Album", L"Yr", },
        std::format(L"{},0,30,30,6", n_width),
        L"C7,NON,C3,C5,C2"
    };

    modes[9] = modes[8];

    modes[0] = modes[8];

    for (size_t idx = 3; idx < 9; idx++)
    {
        modes[idx].ColumnTitles = &columns[idx].titles[0];
        modes[idx].ColumnWidths = columns[idx].width.c_str();
        modes[idx].ColumnTypes = columns[idx].types.c_str();
    }
}

const view_abstract::sort_modes_t& album_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Track Number", SM_EXT, { VK_F9, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t album_tracks_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_EXT) //  by track number
    {
        const auto
            &item1 = static_cast<const track_t*>(data1),
            &item2 = static_cast<const track_t*>(data2);

        if (item1->disc_number == item2->disc_number)
        {
            if (item1->track_number != item2->track_number)
                return item1->track_number < item2->track_number ? -1 : 1;
            return 0;
        }
        return item1->disc_number < item2->disc_number ? -1 : 1;
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
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

std::generator<const track_t&> album_tracks_view::get_tracks()
{
    for (const auto &track: album_tracks) co_yield track;
}

std::vector<wstring> album_tracks_view::get_extra_columns(const track_t& track) const
{
    wstring track_number;

    if (is_multidisc)
    {
        wstring label = std::format(L"{:02}/{:02}", track.track_number, track.disc_number);
        track_number = std::format(L"{: ^7}", label);
    }
    else
    {
        wstring label = std::format(L"{:02}", track.track_number);
        track_number = std::format(L"{: ^4}", label);
    }

    return {
        track_number, // C7 - track number on its album
    };
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
recent_tracks_view::recent_tracks_view(HANDLE panel, api_weak_ptr_t api):
    tracks_base_view(panel, api, get_text(MPanelTracksItemLabel), std::bind(events::show_root, api))
{
    utils::events::start_listening<play_history_observer>(this);
    rebuild_items();
}

recent_tracks_view::~recent_tracks_view()
{
    utils::events::stop_listening<play_history_observer>(this);
    items.clear();
}

config::settings::view_t recent_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

const view_abstract::sort_modes_t& recent_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Played at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
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

std::generator<const track_t&> recent_tracks_view::get_tracks()
{
    for (const auto &track: items) co_yield track;
}

void recent_tracks_view::on_items_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
saved_tracks_view::saved_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy):
    tracks_base_view(panel, api_proxy, get_text(MPanelTracksItemLabel), std::bind(events::show_root, api_proxy))
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
    // sort mode - by Release year; descending; view mode - F3
    return { 0, true, 3 };
}

const view_abstract::sort_modes_t& saved_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, { VK_F9, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t saved_tracks_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME) //  by `saved at` date
    {
        const auto
            &item1 = static_cast<const saved_track_t*>(data1),
            &item2 = static_cast<const saved_track_t*>(data2);

        return item1->added_at.compare(item2->added_at);
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
}

void saved_tracks_view::update_panel_info(OpenPanelInfo *info)
{
    tracks_base_view::update_panel_info(info);
    
    auto *modes = const_cast<PanelMode*>(info->PanelModesArray);

    // adding track number column to some of the view modes

    static const wchar_t* titles_3[] = { L"Saved at", L"Name", L"[E]", L"Length", L"Pop %", };
    modes[3].ColumnTypes = L"C7,NON,C0,C1,C4";
    modes[3].ColumnWidths = L"12,0,3,7,5";
    modes[3].ColumnTitles = titles_3;

    static const wchar_t* titles_4[] = { L"#", L"Name", L"Artist", L"Length" };
    modes[4].ColumnTypes = L"C7,NON,C3,C1";
    modes[4].ColumnWidths = L"12,0,30,7";
    modes[4].ColumnTitles = titles_4;

    static const wchar_t* titles_5[] = { L"#", L"Name", L"Artist", L"Album", L"Type", L"Yr", L"[E]", L"Length", L"Pop %", };
    modes[5].ColumnTypes = L"C7,NON,C3,C5,C6,C2,C0,C1,C4";
    modes[5].ColumnWidths = L"12,0,35,35,6,6,3,7,5";
    modes[5].ColumnTitles = titles_5;

    static const wchar_t* titles_6[] = { L"#", L"Name", L"Artist", L"[E]", L"Length", L"Pop %", };
    modes[6].ColumnTypes = L"C7,NON,C3,C0,C1,C4";
    modes[6].ColumnWidths = L"12,0,30,3,7,5";
    modes[6].ColumnTitles = titles_6;

    static const wchar_t* titles_7[] = { L"#", L"Name", L"Album", L"Yr", L"[E]", L"Length", L"Pop %", };
    modes[7].ColumnTypes = L"C7,NON,C5,C2,C0,C1,C4";
    modes[7].ColumnWidths = L"12,0,30,6,3,7,5";
    modes[7].ColumnTitles = titles_7;

    static const wchar_t* titles_8[] = { L"#", L"Name", L"Artist", L"Album", L"Yr", };
    modes[8].ColumnTypes = L"C7,NON,C3,C5,C2";
    modes[8].ColumnWidths = L"12,0,30,30,6";
    modes[8].ColumnTitles = titles_8;

    modes[9] = modes[8];

    modes[0] = modes[8];
}

bool saved_tracks_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const track_t&> saved_tracks_view::get_tracks()
{
    if (collection->fetch(false, true, 1))
        for (const auto &t: *collection)
            co_yield t;
}

std::vector<wstring> saved_tracks_view::get_extra_columns(const track_t& track) const
{
    const auto &saved_track = static_cast<const saved_track_t&>(track);

    // we take first 10 symbols - it is date, the rest is time
    const auto &saved_at_str = std::format("{:^12}", saved_track.added_at.substr(0, 10));

    return {
        utils::to_wstring(saved_at_str), // C7 - `added at` date
    };
}

void saved_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
}


//-----------------------------------------------------------------------------------------------------------
playing_queue_view::playing_queue_view(HANDLE panel, api_weak_ptr_t api):
    tracks_base_view(panel, api, get_text(MPanelTracksItemLabel), std::bind(events::show_root, api))
{
    utils::events::start_listening<playback_observer>(this);
}

playing_queue_view::~playing_queue_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t playing_queue_view::get_default_settings() const
{
    // sort mode - Unsorted; ascending; view mode - F7
    return { 1, false, 7 };
}

bool playing_queue_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const track_t&> playing_queue_view::get_tracks()
{
    static playing_queue_t playing_queue;
    
    if (auto api = api_proxy.lock())
    {
        playing_queue = api->get_playing_queue();

        // currently playing item
        if (playing_queue.currently_playing)
            co_yield playing_queue.currently_playing;
        
        // queued items
        for (const auto &t: playing_queue.queue)
            co_yield t;
    }
}

const view_abstract::sort_modes_t& playing_queue_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { L"Unsorted", SM_UNSORTED, { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
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
recently_liked_tracks_view::recently_liked_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy):
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
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Saved at", SM_MTIME, { VK_F6, LEFT_CTRL_PRESSED } });
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

std::generator<const track_t&> recently_liked_tracks_view::get_tracks()
{
    // requesting only three pages of the data
    if (collection->fetch(false, true, 3))
        for (const auto &t: *collection)
            co_yield t;
}


//-----------------------------------------------------------------------------------------------------------
user_top_tracks_view::user_top_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy):
    tracks_base_view(panel, api_proxy, get_text(MPanelRecentlyLikedTracksLabel),
                     std::bind(events::show_browse, api_proxy))
{
    if (auto api = api_proxy.lock())
        collection = api->get_user_top_tracks();
}

config::settings::view_t user_top_tracks_view::get_default_settings() const
{
    return { 0, false, 1 };
}

const view_abstract::sort_modes_t& user_top_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { L"Unsorted", SM_UNSORTED, { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

bool user_top_tracks_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const track_t&> user_top_tracks_view::get_tracks()
{
    if (collection && collection->fetch(false, true, 4))
        for (const auto &t: *collection)
            co_yield t;
}

} // namespace ui
} // namespace spotifar