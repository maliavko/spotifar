#include "playlist.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using namespace utils::far3;
using utils::far3::get_text;

playlist_view::playlist_view(api_proxy_ptr api_proxy, const playlist_t &p):
    view_abstract("playlist_view", p.name, std::bind(events::show_saved_playlists, api_proxy)),
    playlist(p),
    api_proxy(api_proxy)
{
    if (auto api = api_proxy.lock())
        collection = api->get_playlist_tracks(p.id);
}

void playlist_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Artist", L"Time", };
    modes[3].ColumnTypes = L"NON,C1,C2";
    modes[3].ColumnWidths = L"0,22,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Name", L"Album", L"Year", };
    modes[4].ColumnTypes = L"NON,C4,C5";
    modes[4].ColumnWidths = L"0,22,6";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Name", L"Added", };
    modes[5].ColumnTypes = L"NON,C0";
    modes[5].ColumnWidths = L"0,22";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view_abstract::items_t& playlist_view::get_items()
{
    static view_abstract::items_t items; items.clear();

    if (!collection || !collection->fetch())
        return items;
    
    for (const auto &t: *collection)
    {
        std::vector<wstring> columns;

        // column C0 - added data
        columns.push_back(std::format(L"{}", utils::to_wstring(t.added_at)));

        // column C1 - artist
        columns.push_back(std::format(L"{}", t.artists[0].name));

        // column C2 - duration
        auto duration = std::chrono::milliseconds(t.duration_ms);
        wstring track_length;
        if (duration < 1h)
            track_length = std::format(L"{:%M:%S}", duration);
        else
            track_length = std::format(L"{:%Hh%M}", duration);
        columns.push_back(track_length.substr(0, 5));

        // column C3 - combined name: Artist Name - Track Name
        columns.push_back(std::format(L"{} - {}", t.artists[0].name, t.name));

        // column C4 - album name
        columns.push_back(std::format(L"{}", t.album.name));

        // column C5 - album year
        columns.push_back(std::format(L"{: ^6}", utils::to_wstring(t.album.get_release_year())));

        items.push_back({
            t.id,
            t.name,
            L"",
            FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<saved_track_t*>(&t)
        });
    }

    return items;
}

const view_abstract::sort_modes_t& playlist_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,    VK_F3 + mods::ctrl },
        { L"Year",      SM_ATIME,   VK_F4 + mods::ctrl },
        { L"Length",    SM_SIZE,    VK_F5 + mods::ctrl },
        { L"Added",     SM_MTIME,   VK_F6 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t playlist_view::get_default_settings() const
{
    return { 1, false, 3 };
}

intptr_t playlist_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const saved_track_t*>(data1),
        &item2 = static_cast<const saved_track_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_SIZE:
            return item1->duration_ms - item2->duration_ms;

        case SM_ATIME:
            return item1->album.release_date.compare(item2->album.release_date);

        case SM_MTIME:
            return item1->added_at.compare(item2->added_at);
    }
    return -2;
}

intptr_t playlist_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_RETURN + utils::keys::mods::shift:
        {
            if (const auto &item = panels::get_current_item(PANEL_ACTIVE))
            {
                if (const auto *user_data = unpack_user_data(item->UserData))
                {
                    if (auto api = api_proxy.lock())
                    {
                        api->start_playback(
                            playlist,
                            *static_cast<const saved_track_t*>(user_data)
                        );
                        return TRUE;
                    }
                }
            }

            log::global->error("An error occured while trying to start playback for "
                "playlist '{}[{}]'", utils::to_string(playlist.name), playlist.id);

            return TRUE;
        }
    }
    return FALSE;
}

} // namespace ui
} // namespace spotifar