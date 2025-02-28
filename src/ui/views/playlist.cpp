#include "playlist.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlist_view::playlist_view(api_abstract *api, const spotify::playlist &p):
    playlist(p),
    api_proxy(api)
{
    for (const auto &t: api_proxy->get_playlist_tracks(playlist.id))
        items.push_back({t.track.id, t.track.name, L"", FILE_ATTRIBUTE_VIRTUAL});
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

intptr_t playlist_view::select_item(const string &track_id)
{
    if (track_id.empty())
    {
        events::show_playlists_view();
        return TRUE;
    }
    
    // TODO: what to do here? start playing?
    // auto playlist = api->get_playlist(playlist_id);

    return FALSE;
}

auto playlist_view::find_processor::get_items() const -> const items_t*
{
    size_t total_tracks = 0;
    string request_url = httplib::append_query_params(
        std::format("/v1/playlists/{}/tracks", playlist_id), {
            { "limit", "1" },
            { "additional_types", "track" },
            { "fields", "total" },
        });

    auto r = api_proxy->get(request_url, utils::http::session);
    if (utils::http::is_success(r->status))
    {
        json data = json::parse(r->body);
        data["total"].get_to(total_tracks);
    }

    static items_t items;
    items.assign({
        // it's a pure fake item, which holds the size of the total amount of followed artists,
        // for the sake of showing it in the item's size column on the panel
        { "", std::format(L"playlist tracks {}", utils::to_wstring(playlist_id)),
            L"", FILE_ATTRIBUTE_VIRTUAL, total_tracks }
    });

    return &items;
}

} // namespace ui
} // namespace spotifar