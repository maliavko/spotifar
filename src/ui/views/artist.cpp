#include "artist.hpp"
#include "album.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artist_view::artist_view(api_abstract *api, const spotify::artist &artist):
    view("artist_view"),
    api_proxy(api),
    artist(artist)
{
}

const wchar_t* artist_view::get_dir_name() const
{
    static wchar_t dir_name[MAX_PATH];
    wcsncpy_s(dir_name, artist.name.c_str(), MAX_PATH);
    return dir_name;
}

const wchar_t* artist_view::get_title() const
{
    return artist.name.c_str();
}

const view::sort_modes_t& artist_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Release Year",  SM_ATIME,   VK_F4 + mods::ctrl },
        { L"Tracks",        SM_SIZE,    VK_F5 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t artist_view::get_default_settings() const
{
    return { 1, false, 3 };
}

intptr_t artist_view::select_item(const user_data_t* data)
{
    if (data == nullptr)
    {
        events::show_artists_view();
        return TRUE;
    }
    
    const album &album = api_proxy->get_album(data->id);
    if (album.is_valid())
    { 
        events::show_album_view(artist, album);
        return TRUE;
    }

    return FALSE;
}

intptr_t artist_view::compare_items(const sort_mode_t &sort_mode,
    const user_data_t *data1, const user_data_t *data2)
{
    const auto
        &item1 = static_cast<const album_user_data_t*>(data1),
        &item2 = static_cast<const album_user_data_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_ATIME:
            return item1->release_year.compare(item2->release_year);

        case SM_SIZE:
            return item1->tracks_total - item2->tracks_total;
    }
    return -2;
}

void artist_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Yr", L"Name", L"Tx", L"Time", L"Type" };
    modes[3].ColumnTypes = L"C0,NON,C2,C5,C1";
    modes[3].ColumnWidths = L"6,0,4,8,6";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Yr", L"Name", L"Tx" };
    modes[4].ColumnTypes = L"C0,NON,C2";
    modes[4].ColumnWidths = L"6,0,4";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Yr", L"Name", L"Tx", L"Type", L"Release" };
    modes[5].ColumnTypes = L"C0,NON,C2,C1,C4";
    modes[5].ColumnWidths = L"6,0,4,6,10";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { L"Yr", L"Name", L"Artists" };
    modes[6].ColumnTypes = L"C0,NON,Z";
    modes[6].ColumnWidths = L"6,0,0";
    modes[6].ColumnTitles = titles_6;

    modes[7] = modes[6];
    modes[7].Flags = PMFLAGS_FULLSCREEN;

    modes[8] = modes[5]; // the same as 5th, but not fullscreen
    modes[8].Flags &= ~PMFLAGS_FULLSCREEN;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

const view::items_t* artist_view::get_items()
{
    static items_t items; items.clear();

    for (const auto &a: api_proxy->get_artist_albums(artist.id))
    {
        std::vector<wstring> columns;

        // TODO: saved-or-not column?
        
        // column C0 - release year
        columns.push_back(std::format(L"{: ^6}", utils::to_wstring(a.get_release_year())));
        
        // column C1 - album type
        columns.push_back(std::format(L"{: ^6}", a.get_type_abbrev()));
        
        // column C2 - total tracks
        columns.push_back(std::format(L"{:3}", a.total_tracks));

        // column C3 - full name
        columns.push_back(a.get_user_name());

        // column C4 - full release date
        columns.push_back(std::format(L"{: ^10}", utils::to_wstring(a.release_date)));

        // column C5 - album length
        size_t total_length_ms = 0;
        auto requester = album_tracks_requester(a.id);
        if (api_proxy->is_request_cached(requester.url))
        {
            for (const auto &t: api_proxy->get_album_tracks(a.id))
                total_length_ms += t.duration_ms;
        }

        // TODO: column C6 - copyrights
        // album popularity
        // music label
        // external ids

        if (total_length_ms > 0)
            columns.push_back(std::format(L"{:%T >8}", std::chrono::milliseconds(total_length_ms)));
        else
            columns.push_back(L"");

        // list of artists is used as a description field
        std::vector<wstring> artists_names;
        std::transform(a.artists.cbegin(), a.artists.cend(), back_inserter(artists_names),
            [](const auto &a) { return a.name; });

        items.push_back({
            a.id,
            a.name,
            utils::string_join(artists_names, L", "),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            new album_user_data_t{ a.id, a.name, a.get_release_year(), a.total_tracks, },
            free_user_data<album_user_data_t>
        });
    }
    return &items;
}

bool artist_view::request_extra_info(const view::user_data_t* data)
{
    if (data != nullptr)
        return album_tracks_requester(data->id)(api_proxy);

    return false;
}

intptr_t artist_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_F4:
        {
            auto item = utils::far3::panels::get_current_item(PANEL_ACTIVE);
            if (item != nullptr)
            {
                // TODO: the view cannot unpack user data, as it was packed by panel.
                // this over the idea, that the view should already receive here 
                // an event together with the cursor item

                // utils::log::global->info("An album is being picked for playback, {}", );
                // api_proxy->start_playback(album::make_uri(data->id));
            }
            else
                utils::log::global->error("There is an error occured while getting a current panel item");

            return TRUE;
        }
    }
    return FALSE;
}

} // namespace ui
} // namespace spotifar