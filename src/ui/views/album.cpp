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
        album.get_user_name()).c_str(), MAX_PATH);
    return dir_name;
}

const wchar_t* album_view::get_title() const
{
    return get_dir_name();
}

const view::items_t* album_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &track: api_proxy->get_album_tracks(album.id))
    {
        std::vector<wstring> columns;
        
        // column C0 - track number
        //columns.push_back(std::format(L"{: ^6}", utils::to_wstring(a.get_release_year())));

        // artist, to distinguis feat.
        // explicit
        // check popularity; in artist's top pick it exists

        items.push_back({
            track.id,
            std::format(L"{:02}. {}", track.track_number, track.name),
            L"",
            FILE_ATTRIBUTE_VIRTUAL, 0,
            columns
        });
    }

    return &items;
}

void album_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Yr", L"Name", L"Ts", L"Time", L"Type" };
    modes[3].ColumnTypes = L"C0,NON,C2,C5,C1";
    modes[3].ColumnWidths = L"6,0,4,8,6";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

intptr_t album_view::select_item(const string &track_id)
{
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