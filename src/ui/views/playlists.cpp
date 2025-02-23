#include "playlists.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlists_view::playlists_view(spotify::api_abstract *api):
    api_proxy(api)
{
}

const wchar_t* playlists_view::get_dir_name() const
{
    return get_title();
}

const wchar_t* playlists_view::get_title() const
{
    return get_text(MPanelPlaylistsItemLabel);
}

view::items_t playlists_view::get_items()
{
    items_t result;
    for (const auto &p: api_proxy->get_playlists())
        result.push_back({p.id, p.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

intptr_t playlists_view::select_item(const string &playlist_id)
{
    if (playlist_id.empty())
    {
        events::show_root_view();
        return TRUE;
    }
    
    auto playlist = api_proxy->get_playlist(playlist_id);
    if (playlist.is_valid())
    {
        events::show_playlist_view(playlist);
        return TRUE;
    }

    return FALSE;
}

} // namespace ui
} // namespace spotifar