#include "album.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

album_view::album_view(spotify::api_abstract *api, const spotify::artist &artist, const spotify::album &album):
    view(album.get_user_name()),
    api_proxy(api),
    album(album),
    artist(artist)
{
    rebuild_items();
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

void album_view::rebuild_items()
{
    items.clear();

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

} // namespace ui
} // namespace spotifar