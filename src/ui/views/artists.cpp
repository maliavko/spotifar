#include "artists.hpp"
#include "root.hpp"
#include "artist.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artists_view::artists_view(spotify::api_abstract *api):
    view(get_text(MPanelArtistsItemLabel)),
    api_proxy(api)
{
}

view::view_items_t artists_view::get_items()
{
    view_items_t result;
    for (const auto &a: api_proxy->get_followed_artists())
        result.push_back({a.id, a.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

intptr_t artists_view::select_item(const string &artist_id)
{
    if (artist_id.empty())
    {
        events::show_root_view();
        return TRUE;
    }
    
    const artist &artist = api_proxy->get_artist(artist_id);
    if (artist.is_valid())
    {
        events::show_artist_view(artist);
        return TRUE;
    }
    
    return FALSE;
}

} // namespace ui
} // namespace spotifar