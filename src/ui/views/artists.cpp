#include "artists.hpp"
#include "root.hpp"
#include "artist.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artists_view::artists_view(spotify::api *api):
    view(get_text(MPanelArtistsItemLabel)),
    api(api)
{
}

view::view_items_t artists_view::get_items()
{
    view_items_t result;
    for (const auto &a: api->get_library().get_followed_artists())
        result.push_back({a->id, a->name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    return result;
}

std::shared_ptr<view> artists_view::select_item(const string &artist_id)
{
    if (artist_id.empty())
        return root_view::build(api);
    
    const artist *a = api->get_library().get_artist(artist_id);
    if (a != nullptr)
        return artist_view::build(api, *a);
    
    return nullptr;
}

std::shared_ptr<artists_view> artists_view::build(spotify::api *api)
{
    return std::make_shared<artists_view>(api);
}

} // namespace ui
} // namespace spotifar