#include "album.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

album_view::album_view(spotify::api_abstract *api, const spotify::artist &ar, const spotify::album &al):
    api_proxy(api),
    album(al),
    artist(ar)
{
}

const wchar_t* album_view::get_dir_name() const
{
    static wchar_t dir_name[MAX_PATH];
    wcsncpy_s(dir_name, utils::strip_invalid_filename_chars(
        album.name).c_str(), MAX_PATH);
    return dir_name;
}

const wchar_t* album_view::get_title() const
{
    return get_dir_name();
}

const view::items_t* album_view::get_items()
{
    static view::items_t items; items.clear();

    const auto &tracks = api_proxy->get_album_tracks(album.id);

    // performing an extra loop across all the tracks to determine the total number of
    // the discs on the album
    size_t discs_number = 0;
    for (const auto &track: tracks)
        if (track.disc_number > discs_number)
            discs_number += 1;

    for (const auto &track: tracks)
    {
        std::vector<wstring> columns;
        
        // column C0 - track number
        // eaither just 01,02 or in a multisdics case - 1/02, 1/02 etc.
        wstring track_number = std::format(L"{:02}", track.track_number);
        if (discs_number > 1)
            track_number = std::format(L"{}/{}", track.disc_number, track_number);
        columns.push_back(std::format(L"{: ^6}", track_number));

        // column C1 - is explicit lyrics
        columns.push_back(track.is_explicit ? L"[E]" : L"");

        // column C2 - duration
        auto duration = std::chrono::milliseconds(track.duration_ms);
        wstring track_length;
        if (duration < 1h)
            track_length = std::format(L"{:%M:%S}", duration);
        else
            track_length = std::format(L"{:%Hh%M}", duration);
        columns.push_back(track_length.substr(0, 5));

        // list of artists is used as a description field
        std::vector<wstring> artists_names;
        std::transform(track.artists.cbegin(), track.artists.cend(), back_inserter(artists_names),
            [](const auto &a) { return a.name; });

        items.push_back({
            track.id,
            track.name,
            utils::string_join(artists_names, L", "),
            FILE_ATTRIBUTE_VIRTUAL, 0,
            columns
        });
    }

    return &items;
}

void album_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"##", L"Name", L"[E]", L"Time" };
    modes[3].ColumnTypes = L"C0,NON,C1,C2";
    modes[3].ColumnWidths = L"6,0,3,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

intptr_t album_view::select_item(const SetDirectoryInfo *info)
{
    auto track_id = view::user_data_t::unpack(info->UserData)->id;
    if (track_id.empty())
    {
        events::show_artist_view(artist);
        return TRUE;
    }
    else
    {
        api_proxy->start_playback(album.id, track_id);
        return TRUE; // TODO: or False as nothing has been changed on the panel
    }
    
    return FALSE;
}

} // namespace ui
} // namespace spotifar