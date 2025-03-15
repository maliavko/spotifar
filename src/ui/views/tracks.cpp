#include "tracks.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

tracks_base_view::tracks_base_view(spotify::api_abstract *api, const string &view_uid):
    view(view_uid),
    api_proxy(api)
{
}

const view::sort_modes_t& tracks_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Track Number",  SM_EXT,     VK_F4 + mods::ctrl },
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Duration",      SM_SIZE,    VK_F5 + mods::ctrl },
    };
    return modes;
}

void tracks_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    // TODO: if there is no multidics, the first column can be shorter?
    // or make it the same to Spotify - no disc number, just sorting
    static const wchar_t* titles_3[] = { L"##", L"Name", L"[E]", L"Time" };
    modes[3].ColumnTypes = L"C2,NON,C0,C1";
    modes[3].ColumnWidths = L"7,0,3,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view::items_t* album_tracks_view::get_items()
{
    static view::items_t items; items.clear();

    const auto &tracks = api_proxy->get_album_tracks(album.id);

    // performing an extra loop across all the tracks to determine the total number of
    // the discs on the album
    size_t discs_number = 0;
    for (const auto &track: tracks)
        if (track.disc_number > discs_number)
            discs_number += 1;

    for (const auto &track: tracks)
    {
        std::vector<wstring> columns;
        pack_custom_columns(columns, track);
        
        // column C2 - track number
        // eaither just 01,02 or in a multisdics case - 1/02, 1/02 etc.
        wstring track_number = std::format(L"{:02}", track.track_number);
        if (discs_number > 1)
            track_number = std::format(L"{:02}/{}", track.disc_number, track_number);
        columns.push_back(std::format(L"{: ^7}", track_number));

        // list of artists is used as a description field
        std::vector<wstring> artists_names;
        std::transform(track.artists.cbegin(), track.artists.cend(), back_inserter(artists_names),
            [](const auto &a) { return a.name; });

        items.push_back({
            track.id,
            track.name,
            utils::string_join(artists_names, L", "),
            FILE_ATTRIBUTE_VIRTUAL,
            columns,
            new track_user_data_t{ track.id, track.name, track_number, track.duration_ms, },
            free_user_data<track_user_data_t>
        });
    }

    return &items;
}

intptr_t tracks_base_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
        return goto_root_folder();
    
    return FALSE;
}

intptr_t tracks_base_view::compare_items(const sort_mode_t &sort_mode,
    const user_data_t *data1, const user_data_t *data2)
{
    const auto
        &item1 = static_cast<const track_user_data_t*>(data1),
        &item2 = static_cast<const track_user_data_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_EXT:
            return item1->track_number.compare(item2->track_number);

        case SM_SIZE:
            return item1->duration_ms - item2->duration_ms;
    }
    return -2;
}

void tracks_base_view::pack_custom_columns(std::vector<wstring> &columns, const simplified_track &track)
{
    // column C0 - is explicit lyrics
    columns.push_back(track.is_explicit ? L" * " : L"");

    // column C1 - duration
    auto duration = std::chrono::milliseconds(track.duration_ms);
    wstring track_length;
    if (duration < 1h)
        track_length = std::format(L"{:%M:%S}", duration);
    else
        track_length = std::format(L"{:%Hh%M}", duration);
    columns.push_back(track_length.substr(0, 5));
}

intptr_t tracks_base_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_RETURN + utils::keys::mods::shift:
        {
            auto item = utils::far3::panels::get_current_item(PANEL_ACTIVE);
            if (item != nullptr)
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    utils::log::global->info("Starting playback from the tracks view, {}", user_data->id);
                    if (start_playback(user_data->id))
                    {
                        events::show_player_dialog();
                        return TRUE;
                    }
                }
            }
            else
                utils::log::global->error("There is an error occured while getting a current panel item");

            return TRUE;
        }
    }
    return FALSE;
}


album_tracks_view::album_tracks_view(spotify::api_abstract *api, const spotify::album &album):
    tracks_base_view(api, "album_tracks_view"),
    album(album)
{
}

const wstring& album_tracks_view::get_dir_name() const
{
    return album.name;
}

config::settings::view_t album_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

bool album_tracks_view::goto_root_folder()
{
    if (album.artists.size() > 0)
    {
        const auto &artist = api_proxy->get_artist(album.artists[0].id);
        if (artist.is_valid())
            events::show_artist_view(api_proxy, artist);
        return true;
    }
    return false;
}

bool album_tracks_view::start_playback(const string &track_id)
{
    api_proxy->start_playback(album.get_uri(), track::make_uri(track_id));
    return true;
}

} // namespace ui
} // namespace spotifar