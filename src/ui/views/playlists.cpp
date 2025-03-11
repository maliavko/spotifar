#include "playlists.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

playlists_view::playlists_view(spotify::api_abstract *api):
    view("playlists_view"),
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

intptr_t playlists_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
    {
        events::show_root_view();
        return TRUE;
    }

    const auto &playlist = api_proxy->get_playlist(data->id);
    if (playlist.is_valid())
    {
        events::show_playlist_view(playlist);
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
    info->PanelModesNumber = ARRAYSIZE(modes);
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
            new user_data_t{ p.id, p.name, }, free_user_data<user_data_t>
        });
    }

    return &items;
}

bool playlists_view::request_extra_info(const user_data_t* data)
{
    if (data != nullptr)
        return playlist_tracks_requester(data->id)(api_proxy);

    return false;
}

} // namespace ui
} // namespace spotifar