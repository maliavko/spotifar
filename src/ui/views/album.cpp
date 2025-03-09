#include "album.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

album_view::album_view(spotify::api_abstract *api, const spotify::artist &ar, const spotify::album &al):
    view("album_view"),
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

const view::sort_modes_t& album_view::get_sort_modes() const
{
    using namespace utils::far3::keys;
    static sort_modes_t modes = {
        { L"Number",        SM_EXT,     VK_F4 + mods::ctrl },
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Duration",      SM_SIZE,    VK_F5 + mods::ctrl },
    };
    return modes;
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
        columns.push_back(track.is_explicit ? L" * " : L"");

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
            columns,
            new track_user_data_t{ track.id, track.name, track_number, track.duration_ms, },
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

intptr_t album_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
    {
        events::show_artist_view(artist);
        return TRUE;
    }

    api_proxy->start_playback(album.id, data->id);
    return TRUE; // TODO: or False as nothing has been changed on the panel
}

intptr_t album_view::compare_items(const sort_mode_t &sort_mode,
    const user_data_t *data1, const user_data_t *data2)
{
    const auto
        &item1 = static_cast<const track_user_data_t*>(data1),
        &item2 = static_cast<const track_user_data_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_EXT:
            return item1->track_number.compare(item2->track_number);

        case SM_SIZE:
            return item1->duration_ms - item2->duration_ms;
    }
    return -2;
}
    
FARPANELITEMFREECALLBACK album_view::get_free_user_data_callback()
{
    return track_user_data_t::free;
}

void WINAPI album_view::track_user_data_t::free(void *const user_data,
    const FarPanelItemFreeInfo *const info)
{
    delete reinterpret_cast<const track_user_data_t*>(user_data);
}

} // namespace ui
} // namespace spotifar