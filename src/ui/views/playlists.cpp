#include "playlists.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlists_view::playlists_view(api_abstract *api):
    view("playlists_view", std::bind(events::show_collections, api)),
    api_proxy(api)
{
}

const wstring& playlists_view::get_dir_name() const
{
    static wstring dir_name(get_text(MPanelPlaylistsItemLabel));
    return dir_name;
}

const view::sort_modes_t& playlists_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name", SM_NAME, VK_F3 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t playlists_view::get_default_settings() const
{
    return { 0, false, 3 };
}

intptr_t playlists_view::select_item(const data_item_t* data)
{
    const auto &playlist = api_proxy->get_playlist(data->id);
    if (playlist.is_valid())
    {
        events::show_playlist(api_proxy, playlist);
        return TRUE;
    }

    return FALSE;
}

void playlists_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Tracks" };
    modes[3].ColumnTypes = L"NON,C0";
    modes[3].ColumnWidths = L"0,6";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view::items_t* playlists_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &p: api_proxy->get_playlists())
    {
        // column C0 - total playlist's tracks count
        wstring tracks_count = L"";
        auto requester = playlist_tracks_requester(p.id);
        if (api_proxy->is_request_cached(requester.get_url()) && requester(api_proxy))
            tracks_count = std::format(L"{: >6}", (requester.get_total()));
        
        items.push_back({
            p.id, p.name, L"",
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            {
                tracks_count
            },
            const_cast<simplified_playlist_t*>(&p)
        });
    }

    return &items;
}

bool playlists_view::request_extra_info(const data_item_t* data)
{
    if (data != nullptr)
        return playlist_tracks_requester(data->id)(api_proxy);

    return false;
}

} // namespace ui
} // namespace spotifar