#include "playlists.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static string make_request_url(size_t limit)
{
    return httplib::append_query_params("/v1/me/playlists", {
        { "limit", std::to_string(limit) }
    });
}

playlists_view::playlists_view(spotify::api_abstract *api):
    api_proxy(api)
{
    for (const auto &p: get_playlists())
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

auto playlists_view::find_processor::get_items() const -> const items_t*
{
    size_t total_playlists = 0;
    
    auto r = api_proxy->get(make_request_url(1), utils::http::session);
    if (utils::http::is_success(r->status))
    {
        json data = json::parse(r->body);
        data["total"].get_to(total_playlists);
    }
    
    static items_t items;
    items.assign({
        // it's a pure fake item, which holds the size of the total amount of followed artists,
        // for the sake of showing it in the item's size column on the panel
        { "", L"user playlists", L"", FILE_ATTRIBUTE_VIRTUAL, total_playlists }
    });

    return &items;
}

simplified_playlists_t playlists_view::get_playlists()
{
    simplified_playlists_t result;
    json request_url = make_request_url(50);
    
    do
    {
        auto r = api_proxy->get(request_url, utils::http::session);
        if (utils::http::is_success(r->status))
        {
            json data = json::parse(r->body);
            request_url = data["next"];

            const auto &playlists = data["items"].get<simplified_playlists_t>();
            result.insert(result.end(), playlists.begin(), playlists.end());
        }
    }
    while (!request_url.is_null());

    return result;
}

} // namespace ui
} // namespace spotifar