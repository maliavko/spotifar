#include "playlists.hpp"
#include "ui/events.hpp"
#include "ui/views/playlist.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlists_view::playlists_view(spotify::api_abstract *api):
    api_proxy(api)
{
    for (const auto &p: api_proxy->get_playlists())
        items.push_back({p.id, p.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
}

const wchar_t* playlists_view::get_dir_name() const
{
    return get_title();
}

const wchar_t* playlists_view::get_title() const
{
    return get_text(MPanelPlaylistsItemLabel);
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

auto playlists_view::get_find_processor(const string &playlist_id) -> std::shared_ptr<view::find_processor>
{
    if (!playlist_id.empty())
        return std::make_shared<playlist_view::find_processor>(api_proxy, playlist_id);
    
    return nullptr;
}

auto playlists_view::find_processor::get_items() const -> const items_t*
{
    size_t total_playlists = 0;
    
    auto requester = user_playlists_requester(1);
    if (requester(api_proxy))
        total_playlists = requester.get_total();
    
    static items_t items;
    items.assign({
        // it's a pure fake item, which holds the size of the total amount of followed artists,
        // for the sake of showing it in the item's size column on the panel
        { "", L"user playlists", L"", FILE_ATTRIBUTE_VIRTUAL, total_playlists }
    });

    return &items;
}

} // namespace ui
} // namespace spotifar