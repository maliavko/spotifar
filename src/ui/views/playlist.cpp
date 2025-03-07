#include "playlist.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlist_view::playlist_view(api_abstract *api, const spotify::playlist &p):
    view("playlist_view"),
    playlist(p),
    api_proxy(api)
{
}

const wchar_t* playlist_view::get_dir_name() const
{
    static wchar_t dir_name[MAX_PATH];
    wcsncpy_s(dir_name, utils::strip_invalid_filename_chars(playlist.name).c_str(), MAX_PATH);
    return dir_name;
}

const wchar_t* playlist_view::get_title() const
{
    return playlist.name.c_str();
}

void playlist_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Artist", L"Time", };
    modes[3].ColumnTypes = L"NON,C1,C2";
    modes[3].ColumnWidths = L"0,22,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Name", L"Album", L"Year", };
    modes[4].ColumnTypes = L"NON,C4,C5";
    modes[4].ColumnWidths = L"0,22,6";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Name", L"Added", };
    modes[5].ColumnTypes = L"NON,C0";
    modes[5].ColumnWidths = L"0,22";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

const view::items_t* playlist_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &t: api_proxy->get_playlist_tracks(playlist.id))
    {
        std::vector<wstring> columns;

        // column C0 - added data
        columns.push_back(std::format(L"{}", utils::to_wstring(t.added_at)));

        // column C1 - artist
        columns.push_back(std::format(L"{}", t.track.artists[0].name));

        // column C2 - duration
        auto duration = std::chrono::milliseconds(t.track.duration_ms);
        wstring track_length;
        if (duration < 1h)
            track_length = std::format(L"{:%M:%S}", duration);
        else
            track_length = std::format(L"{:%Hh%M}", duration);
        columns.push_back(track_length.substr(0, 5));

        // column C3 - combined name: Artist Name - Track Name
        columns.push_back(std::format(L"{} - {}", t.track.artists[0].name, t.track.name));

        // column C4 - album name
        columns.push_back(std::format(L"{}", t.track.album.name));

        // column C5 - album year
        columns.push_back(std::format(L"{: ^6}", utils::to_wstring(t.track.album.get_release_year())));

        items.push_back({
            t.track.id,
            t.track.name,
            L"",
            FILE_ATTRIBUTE_VIRTUAL,
            0,
            columns,
            new playlist_track_user_data_t{
                t.track.id, t.track.name, t.added_at, t.track.duration_ms,
                t.track.album.name, t.track.album.get_release_year() },
        });
    }

    return &items;
}

intptr_t playlist_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
    {
        events::show_playlists_view();
        return TRUE;
    }

    const auto &track_id = data->id;
    
    // TODO: what to do here? start playing?
    // auto playlist = api->get_playlist(playlist_id);

    return FALSE;
}

const view::sort_modes_t* playlist_view::get_sort_modes()
{
    using namespace utils::far3::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Added",         SM_MTIME,   VK_F4 + mods::ctrl },
        { L"Length",        SM_SIZE,    VK_F5 + mods::ctrl },
        { L"Year",          SM_ATIME,   VK_F6 + mods::ctrl },
    };
    return &modes;
}

intptr_t playlist_view::compare_items(view::sort_mode_t sort_mode,
    const user_data_t *data1, const user_data_t *data2)
{
    const auto
        &item1 = static_cast<const playlist_track_user_data_t*>(data1),
        &item2 = static_cast<const playlist_track_user_data_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_SIZE:
            return item1->duration_ms - item2->duration_ms;

        case SM_ATIME:
            return item1->album_release_year.compare(item2->album_release_year);

        case SM_MTIME:
            return item1->added_at.compare(item2->added_at);
    }
    return -2;
}
    
FARPANELITEMFREECALLBACK playlist_view::get_free_user_data_callback()
{
    return playlist_track_user_data_t::free;
}

void WINAPI playlist_view::playlist_track_user_data_t::free(void *const user_data,
    const FarPanelItemFreeInfo *const info)
{
    delete reinterpret_cast<const playlist_track_user_data_t*>(user_data);
}

} // namespace ui
} // namespace spotifar