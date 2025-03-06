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

    static wstring column_name;
    
    column_name = L"Name";
    if (sort_mode == 1)
        column_name += is_desc ?  L'▼' : L'▲';

    static const wchar_t* titles_3[] = { column_name.c_str(), L"Albums", L"Followers", L"Pop %" };
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

    static const wchar_t* titles_5[] = { column_name.c_str(), L"Albums", L"Followers", L"Pop %", L"Genre" };
    modes[5].ColumnTypes = L"NON,C3,C0,C1,C2";
    modes[5].ColumnWidths = L"0,6,9,5,25";
    modes[5].ColumnTitles = titles_5;
    modes[5].StatusColumnTypes = NULL;
    modes[5].StatusColumnWidths = NULL;
    modes[5].Flags = PMFLAGS_FULLSCREEN;

    static const wchar_t* titles_6[] = { column_name.c_str(), L"Genres" };
    modes[6].ColumnTitles = titles_6;
    modes[6].StatusColumnTypes = NULL;
    modes[6].StatusColumnWidths = NULL;

    static const wchar_t* titles_7[] = { column_name.c_str(), L"Albums", L"Genres" };
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
            column_data,
            new artist_user_data_t{ a.id, a.name, a.popularity, a.followers_total, },
        });
    }

    return &items;
}

void artists_view::free_user_data(void *const user_data)
{
    delete reinterpret_cast<const artist_user_data_t*>(user_data);
}

intptr_t artists_view::select_item(const SetDirectoryInfo *info)
{
    if (info->UserData.Data == nullptr)
    {
        events::show_root_view();
        return TRUE;
    }
    
    auto artist_id = artist_user_data_t::unpack(info->UserData)->id;
    const auto &artist = api_proxy->get_artist(artist_id);
    if (artist.is_valid())
    {
        events::show_artist_view(artist);
        return TRUE;
    }
    
    return FALSE;
}

bool artists_view::request_extra_info(const PluginPanelItem *item)
{
    auto artist_id = artist_user_data_t::unpack(item->UserData)->id;
    if (!artist_id.empty())
        return artist_albums_requester(artist_id)(api_proxy);

    return false;
}

intptr_t artists_view::process_key_input(int combined_key)
{
    using namespace utils::far3::keys;

    switch (combined_key)
    {
        case VK_F3 + mods::ctrl:
            if (sort_mode == 1)
                is_desc = !is_desc;
            else
                is_desc = false;
            sort_mode = 1;
            utils::far3::panels::set_sort_mode(PANEL_ACTIVE, SM_USER, is_desc);
            return TRUE;
        case VK_F4 + mods::ctrl:
            if (sort_mode == 2)
                is_desc = !is_desc;
            else
                is_desc = true;
            sort_mode = 2;
            utils::far3::panels::set_sort_mode(PANEL_ACTIVE, SM_USER, is_desc);
            return TRUE;
        case VK_F5 + mods::ctrl:
            if (sort_mode == 3)
                is_desc = !is_desc;
            else
                is_desc = true;
            sort_mode = 3;
            utils::far3::panels::set_sort_mode(PANEL_ACTIVE, SM_USER, is_desc);
            return TRUE;
    }
    return FALSE;
}

intptr_t artists_view::compare_items(const CompareInfo *info)
{
    const auto
        &item1 = artist_user_data_t::unpack(info->Item1->UserData),
        &item2 = artist_user_data_t::unpack(info->Item2->UserData);

    if (sort_mode == 1)
        return item1->name.compare(item2->name);

    if (sort_mode == 2)
        return item1->popularity - item2->popularity;

    if (sort_mode == 3)
        return item1->followers_count - item2->followers_count;
    
    return -2;
}
    
FARPANELITEMFREECALLBACK artists_view::get_free_user_data_callback()
{
    return artist_user_data_t::free;
}

} // namespace ui
} // namespace spotifar