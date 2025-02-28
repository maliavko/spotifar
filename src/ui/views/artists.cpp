#include "artists.hpp"
#include "root.hpp"
#include "artist.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artists_view::artists_view(api_abstract *api):
    api_proxy(api)
{
    for (const auto &a: api_proxy->get_followed_artists())
    {
        std::vector<wstring> column_data;

        std::wstringstream followers;
        followers << std::setw(6) << std::setprecision(4);

        // column C0 - followers count
        if (a.followers_total < 1000)
            followers << a.followers_total;
        else if (a.followers_total < 1000000)
            followers << a.followers_total / 1000.0 << L" K";
        else if (a.followers_total < 1000000000)
            followers << a.followers_total / 1000000.0 << L" M";
        else if (a.followers_total < 1000000000000)
            followers << a.followers_total / 1000000000.0 << L" B";
        column_data.push_back(followers.str());

        // column C1 - popularity
        column_data.push_back(std::to_wstring(a.popularity));

        // column C2 - genres
        column_data.push_back(utils::to_wstring(utils::string_join(a.genres, ", ")));

        items.push_back({
            a.id,
            a.name,
            L"",
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            0,
            column_data
        });
    }
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
    static const wchar_t* titles[] = {L"Name", L"Followers", L"%"};

    static PanelMode modes[1];
    modes[0].ColumnTypes = L"N,C0";
    modes[0].ColumnWidths = L"20,0";
    modes[0].ColumnTitles = titles;
    modes[0].StatusColumnTypes = L"N,C0";
    modes[0].StatusColumnWidths = L"20,0";

    info->PanelModesArray = modes;
    info->PanelModesNumber = ARRAYSIZE(modes);
}

intptr_t artists_view::select_item(const string &artist_id)
{
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

auto artists_view::get_find_processor(const string &artist_id) -> std::shared_ptr<view::find_processor>
{
    if (!artist_id.empty())
        return std::make_shared<artist_view::find_processor>(api_proxy, artist_id);
    
    return nullptr;
}

auto artists_view::find_processor::get_items() const -> const items_t*
{
    size_t total_artists = 0;
    string request_url = httplib::append_query_params("/v1/me/following", {
        { "type", "artist" },
        { "limit", "1" },
    });

    auto r = api_proxy->get(request_url, utils::http::session);
    if (utils::http::is_success(r->status))
    {
        json data = json::parse(r->body)["artists"];
        data["total"].get_to(total_artists);
    }

    static items_t items;
    items.assign({
        // it's a pure fake item, which holds the size of the total amount of followed artists,
        // for the sake of showing it in the item's size column on the panel
        { "", L"followed artists", L"", FILE_ATTRIBUTE_VIRTUAL, total_artists }
    });

    const auto &result = api_proxy->request(followed_artists_request());

    return &items;
}

} // namespace ui
} // namespace spotifar