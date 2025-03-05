#include "artists.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artists_view::artists_view(api_abstract *api):
    api_proxy(api)
{
}

const wchar_t* artists_view::get_dir_name() const
{
    return get_title();
}

const wchar_t* artists_view::get_title() const
{
    return get_text(MPanelArtistsItemLabel);
}

void artists_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Albums", L"Followers", L"Pop %" };
    modes[3].ColumnTypes = L"NON,C3,C0,C1";
    modes[3].ColumnWidths = L"0,6,9,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    modes[4].ColumnTypes = L"NON,C3";
    modes[4].ColumnWidths = L"0,6";
    modes[4].ColumnTitles = titles_3;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    static const wchar_t* titles_5[] = { L"Name", L"Albums", L"Followers", L"Pop %", L"Genre" };
    modes[5].ColumnTypes = L"NON,C3,C0,C1,C2";
    modes[5].ColumnWidths = L"0,6,9,5,25";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { L"Name", L"Genres" };
    modes[6].ColumnTitles = titles_6;
    modes[6].StatusColumnTypes = NULL;
    modes[6].StatusColumnWidths = NULL;

    static const wchar_t* titles_7[] = { L"Name", L"Albums", L"Genres" };
    modes[7].ColumnTitles = titles_7;
    modes[7].StatusColumnTypes = NULL;
    modes[7].StatusColumnWidths = NULL;

    modes[8] = modes[5]; // the same as 5th, but not fullscreen
    modes[8].Flags &= ~PMFLAGS_FULLSCREEN;

    modes[9] = modes[8];

    modes[0] = modes[8];

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

const view::items_t* artists_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &a: api_proxy->get_followed_artists())
    {
        std::vector<wstring> column_data;

        // column C0 - followers count
        if (a.followers_total < 1000000)
            column_data.push_back(std::format(L"{:9}", a.followers_total));
        else if (a.followers_total < 1000000000)
            column_data.push_back(std::format(L"{:7.2f} M", a.followers_total / 1000000.0));
        else if (a.followers_total < 1000000000000)
            column_data.push_back(std::format(L"{:7.2f} B", a.followers_total / 1000000000.0));

        // column C1 - popularity
        column_data.push_back(std::format(L"{:5}", a.popularity));

        // column C2 - first (main?) genre
        column_data.push_back(a.genres.size() > 0 ? utils::to_wstring(a.genres[0]) : L"");
        
        // column C3 - total albums
        wstring albums_count = L"";
        auto requester = artist_albums_requester(a.id);
        if (api_proxy->is_request_cached(requester.get_url()) && requester(api_proxy))
            albums_count = std::format(L"{: >6}", (requester.get_total()));
        column_data.push_back(albums_count);

        items.push_back({
            a.id,
            a.name,
            utils::to_wstring(utils::string_join(a.genres, ", ")),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            0,
            column_data
        });
    }

    return &items;
}

intptr_t artists_view::select_item(const SetDirectoryInfo *info)
{
    auto artist_id = view::user_data_t::unpack(info->UserData)->id;
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

bool artists_view::request_extra_info(const string &artist_id)
{
    if (!artist_id.empty())
        return artist_albums_requester(artist_id)(api_proxy);

    return false;
}

} // namespace ui
} // namespace spotifar