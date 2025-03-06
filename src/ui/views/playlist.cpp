#include "playlist.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlist_view::playlist_view(api_abstract *api, const spotify::playlist &p):
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

const view::items_t* playlist_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &t: api_proxy->get_playlist_tracks(playlist.id))
        items.push_back({
            t.track.id,
            t.track.name,
            L"",
            FILE_ATTRIBUTE_VIRTUAL,
            0,
            {},
            new view::user_data_t{ t.track.id },
        });

    return &items;
}

intptr_t playlist_view::select_item(const SetDirectoryInfo *info)
{
    if (info->UserData.Data == nullptr)
    {
        events::show_playlists_view();
        return TRUE;
    }

    const auto &track_id = view::user_data_t::unpack(info->UserData)->id;
    
    // TODO: what to do here? start playing?
    // auto playlist = api->get_playlist(playlist_id);

    return FALSE;
}

} // namespace ui
} // namespace spotifar