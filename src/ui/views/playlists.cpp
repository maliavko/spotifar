#include "playlists.hpp"
#include "playlist.hpp"
#include "root.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlists_view::playlists_view(spotify::api *api):
    view(get_text(MPanelPlaylistsItemLabel)),
    api(api)
{
}

view::view_items_t playlists_view::get_items()
{
    view_items_t result;
    for (const auto &p: api->get_library().get_playlists())
        result.push_back({p.id, p.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

std::shared_ptr<view> playlists_view::select_item(const string &playlist_id)
{
    if (playlist_id.empty())
        return root_view::build(api);
    
    auto playlist = api->get_library().get_playlist(playlist_id);
    if (playlist.id != invalid_id)
        return playlist_view::build(api, playlist);

    return nullptr;
}

std::shared_ptr<playlists_view> playlists_view::build(spotify::api *api)
{
    return std::make_shared<playlists_view>(api);
}

} // namespace ui
} // namespace spotifar