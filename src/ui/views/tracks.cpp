#include "tracks.hpp"
#include "lng.hpp"
#include "ui/events.hpp"
#include "spotify/requesters.hpp"

namespace spotifar { namespace ui {

using PM = panel_mode_t;
using utils::far3::get_text;
using utils::far3::get_vtext;
namespace panels = utils::far3::panels;

//-----------------------------------------------------------------------------------------------------------
tracks_base_view::tracks_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name):
    view(panel, title, dir_name), api_proxy(api)
{
    utils::events::start_listening<collection_observer>(this);
}

tracks_base_view::~tracks_base_view()
{
    utils::events::stop_listening<collection_observer>(this);
    items.clear();
}

const sort_modes_t& tracks_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarName),       SM_NAME,            { VK_F3, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarRelease),    SM_ATIME,           { VK_F4, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarPopularity), SM_COMPRESSEDSIZE,  { VK_F5, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarAlbum),      SM_OWNER,           { VK_F6, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarArtist),     SM_CHTIME,          { VK_F7, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarDuration),   SM_SIZE,            { VK_F8, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

const items_t& tracks_base_view::get_items()
{
    items.clear();

    for (const auto &track: get_tracks())
    {
        std::vector<wstring> columns;
        
        bool is_selected = false;
        bool is_saved = false;

        if (auto api = api_proxy.lock())
        {
            const auto &pstate = api->get_playback_state();
            auto *library = api->get_library();
            is_selected = pstate.item && pstate.item.id == track.id;

            is_saved = library->is_track_saved(track.id);
        }
        
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

        // column C7 - is saved in collection status
        columns.push_back(is_saved ? L" + " : L"");
        
        // inherited views custom columns
        const auto &extra = get_extra_columns(track);
        columns.insert(columns.end(), extra.begin(), extra.end());

        items.push_back({
            track.id,
            track.name,
            L"",
            FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<track_t*>(&track),
            is_selected
        });
    }
    return items;
}

const panel_modes_t* tracks_base_view::get_panel_modes() const
{
    static const panel_mode_t::column_t
        Name        { L"NON",   get_text(MSortColName),         L"0" },
        NameFixed   { L"NON",   get_text(MSortColName),         L"30" },
        IsExplicit  { L"C0",    get_text(MSortColExplicit),     L"3" },
        Duration    { L"C1",    get_text(MSortColDuration),     L"7" },
        Year        { L"C2",    get_text(MSortColYear),         L"6" },
        Artist      { L"C3",    get_text(MSortColArtist),       L"30" },
        Popularity  { L"C4",    get_text(MSortColPopularity),   L"5" },
        Album       { L"C5",    get_text(MSortColAlbum),        L"30" },
        Type        { L"C6",    get_text(MSortColType),         L"6" },
        IsSaved     { L"C7",    get_text(MSortColSaved),        L"3" },
        TxNumber    { L"C8",    get_text(MSortColTrackNum),     L"4" },
        TxMultNumber{ L"C8",    get_text(MSortColTrackMultiNum),L"7" },
        PlayedAt    { L"C8",    get_text(MSortColPlayedAt),     L"13" },
        SavedAt     { L"C8",    get_text(MSortColSavedAt),      L"13" },
        AddedAt     { L"C8",    get_text(MSortColAddedAt),      L"13" },
        Descr       { L"Z",     get_text(MSortColDescr),        L"0" };
    
    static panel_modes_t modes{
        /* 0 */ PM::dummy(8),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ &Name, &IsSaved, &IsExplicit, &Duration, &Popularity }),
        /* 4 */ PM({ &Name, &Artist, &IsSaved, &Duration }),
        /* 5 */ PM({ &Name, &Artist, &Album, &Type, &Year, &IsSaved, &IsExplicit, &Duration, &Popularity }, true),
        /* 6 */ PM({ &Name, &Artist, &IsSaved, &IsExplicit, &Duration, &Popularity}),
        /* 7 */ PM({ &Name, &Album, &Year, &IsSaved, &IsExplicit, &Duration, &Popularity }, true),
        /* 8 */ PM({ &Name, &Artist, &Album, &IsSaved, &Year }),
        /* 9 */ PM::dummy(8),
    };
    
    return &modes;
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
    using namespace utils::keys;

    switch (combined_key)
    {
        // starting playback for the item or list of selected items
        case VK_F4:
        {
            const auto &ids = get_selected_items();

            if (ids.size() == 1)
            {
                if (const auto &item = panels::get_current_item(get_panel_handle()))
                {
                    if (auto *user_data = unpack_user_data(item->UserData))
                    {
                        if (const auto &track = static_cast<const track_t&>(*user_data))
                            start_playback(track); // each derived view decides on the launching context
                    }
                }
            }
            else
            {
                // if there are several items selected on the panel, we are launching them in the order
                // selected as a range of URIs
                log::global->info("Launching several tracks playback {}", utils::string_join(ids, ","));
                if (auto api = api_proxy.lock())
                {
                    std::vector<string> uris;
                    std::transform(ids.begin(), ids.end(), std::back_inserter(uris),
                        [](const item_id_t &id) { return spotify::track_t::make_uri(id); });

                    api->start_playback(uris);
                }
            }
            return TRUE;
        }

        // like/unlike
        case VK_F8:
        {
            const auto &ids = get_selected_items();

            if (auto api = api_proxy.lock(); api && !ids.empty())
            {
                // what to do - like or unlike - with the whole list  of items
                // we decide based on the first item state
                if (auto *library = api->get_library(); library->is_track_saved(ids[0], true))
                    library->remove_saved_tracks(ids);
                else
                    library->save_tracks(ids);
            }

            return TRUE;
        }

        // redirect to Spotify WEB for the tracks under cursor
        case VK_RETURN + mods::shift:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (const track_t *track = static_cast<const track_t*>(user_data); !track->urls.spotify.empty())
                        utils::open_web_browser(track->urls.spotify);
                }
            }
            return TRUE;
        }

        // PgDown + Ctrl
        // go to the track's origin album: does make sense for all the view
        // except the album's view, for it jut refreshes the panel
        case VK_NEXT + mods::ctrl:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (auto api = api_proxy.lock())
                    {
                        const auto *track = static_cast<const track_t*>(user_data);
                        if (const auto album = api->get_album(track->album.id); *track)
                            events::show_album_tracks(api_proxy, album);
                    }
                }
            }
            return TRUE;
        }

        case VK_F5 + mods::ctrl + mods::alt:
        {
            if (const auto &item = panels::get_current_item(get_panel_handle()))
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    if (auto api = api_proxy.lock())
                    {
                        const auto *track = static_cast<const track_t*>(user_data);

                        // std::ostringstream cmd;

                        // cmd << "C:\\Users\\malia\\AppData\\Local\\Programs\\Python\\Python313\\Scripts\\spotdl.exe download "
                        //     << track->urls.spotify
                        //     << " --auth-token " << api->get_auth_cache().get_access_token()
                        //     << " --generate-lrc";

                        // auto r = std::system(cmd.str().c_str());

                        //std::wostringstream cmd;
                        // cmd << L"C:\\Users\\malia\\AppData\\Local\\Programs\\Python\\Python313\\Scripts\\spotdl.exe download "
                        //     << utils::to_wstring(track->urls.spotify)
                        //     << L" --auth-token " << utils::to_wstring(api->get_auth_cache().get_access_token())
                        //     << L" --generate-lrc";

                        // HANDLE pipe_read = NULL;
                        // HANDLE pipe_write = NULL;
                        // STARTUPINFO si;
                        // PROCESS_INFORMATION pi;

                        // SECURITY_ATTRIBUTES sa_attrs;
                        // ZeroMemory(&sa_attrs, sizeof(sa_attrs));

                        // sa_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
                        // sa_attrs.bInheritHandle = TRUE;
                        // sa_attrs.lpSecurityDescriptor = NULL;

                        // if (!CreatePipe(&pipe_read, &pipe_write, &sa_attrs, 0))
                        // {
                        //     log::librespot->error("CreatePipe failed, {}", utils::get_last_system_error());
                        //     return false;
                        // }

                        // if (!SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0))
                        // {
                        //     log::librespot->error("SetHandleInformation failed, {}", utils::get_last_system_error());
                        //     return false;
                        // }
                        
                        // ZeroMemory(&si, sizeof(si));
                        // si.cb = sizeof(si);
                        // si.hStdError = pipe_write;
                        // si.hStdOutput = pipe_write;
                        // si.dwFlags |= STARTF_USESTDHANDLES;
                        
                        // ZeroMemory(&pi, sizeof(pi));

                        // if(!CreateProcess(
                        //     NULL,
                        //     &cmd.str()[0],
                        //     NULL, // process handle not inheritable
                        //     NULL, // thread handle not inheritable
                        //     TRUE, // set handle inheritance to TRUE
                        //     0,    // https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags
                        //     NULL, // Use parent's environment block
                        //     NULL, // Use parent's starting directory 
                        //     &si,  // Pointer to STARTUPINFO structure
                        //     &pi)  // Pointer to PROCESS_INFORMATION structure
                        // )
                        // {
                        //     log::librespot->error("CreateProcess failed, {}", utils::get_last_system_error());
                        //     return false;
                        // }
                    }
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

const key_bar_info_t* tracks_base_view::get_key_bar_info()
{
    static key_bar_info_t key_bar{};

    auto view_uid = get_uid();

    auto item = utils::far3::panels::get_current_item(get_panel_handle());

    // right after switching the directory on the panel, when we trap here `panels::get_current_item`
    // returns the `item` from  the previous directory. Checking crc32 helps to identify
    // when the panel is refreshed eventually and we can be sure the item is valid
    if (item->CRC32 != view_uid) return nullptr;
    
    if (auto api = api_proxy.lock())
    {
        if (auto *user_data = unpack_user_data(item->UserData))
        {
            if (api->get_library()->is_track_saved(user_data->id))
                key_bar[{ VK_F8, 0 }] = get_text(MUnlike);
            else
                key_bar[{ VK_F8, 0 }] = get_text(MLike);
        }
    }

    return &key_bar;
}

void tracks_base_view::on_tracks_statuses_changed(const item_ids_t &ids)
{
    on_tracks_statuses_received(ids);
}

void tracks_base_view::on_tracks_statuses_received(const item_ids_t &ids)
{
    std::unordered_set<item_id_t> unique_ids(ids.begin(), ids.end());

    const auto &it = std::find_if(items.begin(), items.end(),
        [&unique_ids](item_t &item) { return unique_ids.contains(item.id); });

    // if any of view's tracks are changed, we need to refresh the panel
    if (it != items.end())
        events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
album_tracks_view::album_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy, const simplified_album_t &album):
    tracks_base_view(panel, api_proxy, album.name), album(album)
{
    utils::events::start_listening<playback_observer>(this);
    rebuild_items();

    static const panel_mode_t::column_t
        TxNumber    { L"C8",    get_text(MSortColTrackNum),         L"4" },
        TxMultNumber{ L"C8",    get_text(MSortColTrackMultiNum),    L"7" };

    // rebuild should happen above, for the `is_multidisc` is valid
    panel_modes = *tracks_base_view::get_panel_modes();
    for (size_t i = 3; i < panel_modes_t::MODES_COUNT; i++)
        panel_modes[i].insert_column(is_multidisc ? &TxMultNumber : &TxNumber, 0);

    panel_modes.rebuild();
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

const sort_modes_t& album_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarTrackNum), SM_EXT, { VK_F9, LEFT_CTRL_PRESSED } });
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

bool album_tracks_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        log::global->info("Starting playback in the album {} from track",
            album.get_uri(), track.get_uri());
        
        api->start_playback(album.get_uri(), track.get_uri());
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
        track_number, // C8 - track number on its album
    };
}

void album_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    if (album.id == track.album.id) // the currently playing track is from this album
        events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
recent_tracks_view::recent_tracks_view(HANDLE panel, api_weak_ptr_t api):
    tracks_base_view(panel, api, get_text(MPanelTracks), get_text(MPanelRecents))
{
    utils::events::start_listening<play_history_observer>(this);
    rebuild_items();

    static const panel_mode_t::column_t
        PlayedAt { L"C8", get_text(MSortColPlayedAt), L"13" };

    panel_modes = *tracks_base_view::get_panel_modes();
    for (size_t i = 3; i < panel_modes_t::MODES_COUNT; i++)
        panel_modes[i].insert_column(&PlayedAt, 0);

    panel_modes.rebuild();
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

const sort_modes_t& recent_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarPlayedAt), SM_MTIME, { VK_F9, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t recent_tracks_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_item_t*>(data1),
            &item2 = static_cast<const history_item_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
}

void recent_tracks_view::rebuild_items()
{
    items.clear();

    if (auto api = api_proxy.lock())
        items = api->get_play_history();
}

bool recent_tracks_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        if (const auto &history_track = static_cast<const history_item_t&>(track))
        {
            if (history_track.context.is_empty())
            {
                log::global->warn("History context is not available, launching the standalone "
                    "track {}", history_track.id);
                api->start_playback({ history_track.get_uri() });
            }
            else
            {
                log::global->info("Launching the track {} with the history context {}",
                    history_track.id, history_track.context.uri);
                api->start_playback(history_track.context.uri, history_track.get_uri());
            }
            return true;
        }
    }
    return false;
}

std::generator<const track_t&> recent_tracks_view::get_tracks()
{
    for (const auto &track: items) co_yield track;
}

std::vector<wstring> recent_tracks_view::get_extra_columns(const track_t& track) const
{
    const auto &played_track = static_cast<const history_item_t&>(track);

    wstring formatted_time = utils::to_wstring(played_track.played_at);

    try
    {
        // parsing datetime string, to be able to reformat it
        std::istringstream in{ played_track.played_at };
        utils::clock_t::time_point tp;
        in >> parse("%FT%T", tp); // e.g. 2025-06-01T21:57:59.793Z

        formatted_time = std::format(L"{:%d %b, %H:%M}", std::chrono::current_zone()->to_local(tp));
    }
    catch (const std::runtime_error &ex)
    {
        log::global->warn("Couldn't convert track last played time into "
            "local time: {}. {}", played_track.played_at, ex.what());
    }

    auto context_name = utils::to_wstring(played_track.context.type);
    
    return {
        formatted_time,     // C8 - `played at` date
        context_name,       // C9 - context type
    };
}

void recent_tracks_view::on_history_changed()
{
    rebuild_items();
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
saved_tracks_view::saved_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy):
    tracks_base_view(panel, api_proxy, get_text(MPanelTracks), get_text(MPanelCollection))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_library()->get_saved_tracks();
        repopulate();
    }

    static const panel_mode_t::column_t
        SavedAt { L"C8", get_text(MSortColSavedAt), L"13" };

    panel_modes = *tracks_base_view::get_panel_modes();
    for (size_t i = 3; i < panel_modes_t::MODES_COUNT; i++)
        panel_modes[i].insert_column(&SavedAt, 0);

    panel_modes.rebuild();
    
    utils::events::start_listening<playback_observer>(this);
}

saved_tracks_view::~saved_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

bool saved_tracks_view::repopulate()
{
    return collection->fetch(false, true);
}

config::settings::view_t saved_tracks_view::get_default_settings() const
{
    // sort mode - by Release year; descending; view mode - F3
    return { 0, true, 3 };
}

const sort_modes_t& saved_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarSavedAt), SM_MTIME, { VK_F9, LEFT_CTRL_PRESSED } });
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

bool saved_tracks_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        log::global->info("Starting saved tracks collection playback from track {}", track.get_uri());
        api->start_playback(COLLECTION_URI, track.get_uri());
        return true;
    }
    return false;
}

std::generator<const track_t&> saved_tracks_view::get_tracks()
{
    if (collection)
        for (const auto &track: *collection) co_yield track;
}

std::vector<wstring> saved_tracks_view::get_extra_columns(const track_t& track) const
{
    const auto &saved_track = static_cast<const saved_track_t&>(track);

    // we take first 10 symbols - it is date, the rest is time
    const auto &saved_at_str = std::format("{:^12}", saved_track.added_at.substr(0, 10));

    return {
        utils::to_wstring(saved_at_str), // C8 - `added at` date
    };
}

void saved_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    events::refresh_panel(get_panel_handle());
}

// void saved_tracks_view::on_tracks_statuses_changed(const item_ids_t &ids)
// {
//     // the base handlers update the view only in case some items
//     // were being change, this view should repopulate itself anyway
//     // as it represents the list of saved tracks
//     repopulate();
//     events::refresh_panel(get_panel_handle());
// }


//-----------------------------------------------------------------------------------------------------------
playing_queue_view::playing_queue_view(HANDLE panel, api_weak_ptr_t api):
    tracks_base_view(panel, api, get_text(MPanelPlayingQueue))
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

bool playing_queue_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        const auto &state = api->get_playback_state();
        if (state.context.does_support_offset())
        {
            log::global->info("Launching the track {} with the context {}",
                track.id, state.context.uri);
            api->start_playback(state.context.uri, track.get_uri());
        }
        else
        {
            log::global->warn("The context does not support offsetting, launching the standalone "
                "track {}", track.id);
            api->start_playback({ track.get_uri() });
        }
        return true;
    }
    return false;
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

const sort_modes_t& playing_queue_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarUnsorted), SM_UNSORTED, { VK_F9, LEFT_CTRL_PRESSED } },
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
    tracks_base_view(panel, api_proxy, get_text(MPanelRecentlyLikedTracks), get_text(MPanelRecentlySaved))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_library()->get_saved_tracks();
        collection->fetch(false, true, 3);
    }

    utils::events::start_listening<playback_observer>(this);
}

recently_liked_tracks_view::~recently_liked_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t recently_liked_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

bool recently_liked_tracks_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        log::global->info("Starting saved tracks collection playback from track {}", track.get_uri());
        api->start_playback(COLLECTION_URI, track.get_uri());
        return true;
    }
    return false;
}

const sort_modes_t& recently_liked_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarSavedAt), SM_MTIME, { VK_F9, LEFT_CTRL_PRESSED } });
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
    if (collection)
        for (const auto &t: *collection) co_yield t;
}

void recently_liked_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
user_top_tracks_view::user_top_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy):
    tracks_base_view(panel, api_proxy, get_text(MPanelUserTopTracks), get_text(MPanelUserTopItems))
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_user_top_tracks();
        collection->fetch(false, true, 4);
    }
}

config::settings::view_t user_top_tracks_view::get_default_settings() const
{
    return { 0, false, 1 };
}

const sort_modes_t& user_top_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarUnsorted), SM_UNSORTED, { VK_F9, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

bool user_top_tracks_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        log::global->info("Starting standalone track from user's top {}", track.get_uri());
        api->start_playback({ track.get_uri() });
        return true;
    }
    return false;
}

std::generator<const track_t&> user_top_tracks_view::get_tracks()
{
    if (collection)
        for (const auto &t: *collection) co_yield t;
}

void user_top_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    events::refresh_panel(get_panel_handle());
}


//-----------------------------------------------------------------------------------------------------------
artist_top_tracks_view::artist_top_tracks_view(HANDLE panel, api_weak_ptr_t api_proxy, const artist_t &artist):
    tracks_base_view(panel, api_proxy, get_vtext(MPanelArtistTopTracks, artist.name), artist.name), artist(artist)
{
    if (auto api = api_proxy.lock())
        tracks = api->get_artist_top_tracks(artist.id);
    
    utils::events::start_listening<playback_observer>(this);
}

artist_top_tracks_view::~artist_top_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t artist_top_tracks_view::get_default_settings() const
{
    return { 5, false, 7 };
}

const sort_modes_t& artist_top_tracks_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes[5] = { get_text(MSortBarUnsorted), SM_UNSORTED, { VK_F9, LEFT_CTRL_PRESSED } };
    }
    return modes;
}

bool artist_top_tracks_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        log::global->info("Starting standalone track from artist's top {}", track.get_uri());
        api->start_playback({ track.get_uri() });
        return true;
    }
    return false;
}

std::generator<const track_t&> artist_top_tracks_view::get_tracks()
{
    for (const auto &track: tracks) co_yield track;
}

void artist_top_tracks_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    events::refresh_panel(get_panel_handle());
}


//--------------------------------------------------------------------------------------------------------
playlist_view::playlist_view(HANDLE panel, api_weak_ptr_t api_proxy, const playlist_t &p):
    tracks_base_view(panel, api_proxy, p.name, p.name), playlist(p)
{
    if (auto api = api_proxy.lock())
    {
        collection = api->get_playlist_tracks(p.id);
        collection->fetch();
    }
    
    utils::events::start_listening<playback_observer>(this);

    static const panel_mode_t::column_t
        AddedAt { L"C8", get_text(MSortColAddedAt), L"13" };

    panel_modes = *tracks_base_view::get_panel_modes();
    for (size_t i = 3; i < panel_modes_t::MODES_COUNT; i++)
        panel_modes[i].insert_column(&AddedAt, 0);

    panel_modes.rebuild();
}

playlist_view::~playlist_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

config::settings::view_t playlist_view::get_default_settings() const
{
    return { 0, true, 3 };
}

const sort_modes_t& playlist_view::get_sort_modes() const
{
    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ get_text(MSortBarAddedAt), SM_MTIME, { VK_F9, LEFT_CTRL_PRESSED } });
    }
    return modes;
}

intptr_t playlist_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME) //  by `added at` date
    {
        const auto
            &item1 = static_cast<const saved_track_t*>(data1),
            &item2 = static_cast<const saved_track_t*>(data2);

        return item1->added_at.compare(item2->added_at);
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
}

bool playlist_view::start_playback(const track_t &track)
{
    if (auto api = api_proxy.lock())
    {
        log::global->info("Starting track {} from playlist {}", track.get_uri(), playlist.id);
        api->start_playback(playlist.get_uri(), track.get_uri());
        return true;
    }
    return false;
}

std::generator<const track_t&> playlist_view::get_tracks()
{
    if (collection)
        for (const auto &t: *collection) co_yield t;
}

std::vector<wstring> playlist_view::get_extra_columns(const track_t& track) const
{
    const auto &saved_track = static_cast<const saved_track_t&>(track);

    // we take first 10 symbols - it is date, the rest is time
    const auto &saved_at_str = std::format("{:^12}", saved_track.added_at.substr(0, 10));

    return {
        utils::to_wstring(saved_at_str), // C8 - `added at` date
    };
}

void playlist_view::on_track_changed(const track_t &track, const track_t &prev_track)
{
    events::refresh_panel(get_panel_handle());
}

} // namespace ui
} // namespace spotifar