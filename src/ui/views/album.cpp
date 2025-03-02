#include "album.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

album_view::album_view(spotify::api_abstract *api, const spotify::artist &ar, const spotify::album &al):
    api_proxy(api),
    album(al),
    artist(ar)
{
    for (const auto &track: api_proxy->get_album_tracks(album.id))
    {
        items.push_back({
            track.id,
            std::format(L"{:02}. {}", track.track_number, track.name),
            L"",
            FILE_ATTRIBUTE_VIRTUAL
        });
    }
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

size_t album_view::get_item_idx(const string &item_id)
{
    for (size_t idx = 0; idx < items.size(); idx++)
        if (items[idx].id == item_id)
            return idx;
    return 0;
}

auto album_view::find_processor::get_items() const -> const items_t*
{
    size_t total_tracks = 0;

    auto requester = album_tracks_requester(album_id);
    if (requester(api_proxy))
        total_tracks = requester.get_total();

    static items_t items;
    items.assign({
        // it's a pure fake item, which holds the size of the total amount of followed artists,
        // for the sake of showing it in the item's size column on the panel
        { "", std::format(L"album tracks {} {}", utils::to_wstring(artist_id), utils::to_wstring(album_id)),
            L"", FILE_ATTRIBUTE_VIRTUAL, total_tracks }
    });

    return &items;
}

} // namespace ui
} // namespace spotifar