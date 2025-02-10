#include "playlists.hpp"
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
    // TODO: tmp code
    view_items_t result;
    for (auto& [id, a]: api->get_playlists())
        result.push_back({id, a.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

std::shared_ptr<view> playlists_view::select_item(const string &playlist_id)
{
    if (playlist_id.empty())
        return root_view::build(api);
    
    return NULL;
}

std::shared_ptr<playlists_view> playlists_view::build(spotify::api *api)
{
    return std::make_shared<playlists_view>(api);
}

} // namespace ui
} // namespace spotifar