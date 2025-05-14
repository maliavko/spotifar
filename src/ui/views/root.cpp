#include "root.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
using namespace events;

root_base_view::root_base_view(api_proxy_ptr api, const string &uid, const wstring &title,
                               return_callback_t callback, menu_items_t items):
    view_abstract(uid, title, callback),
    api_proxy(api),
    menu_items(items)
{
}

const view_abstract::key_bar_info_t* root_base_view::get_key_bar_info()
{
    static key_bar_info_t key_bar{
        // example data
        // { { VK_F4, 0 }, get_text(MKeyBarF4) },
    };

    return &key_bar;
}

const view_abstract::info_lines_t* root_base_view::get_info_lines()
{
    static info_lines_t lines{
        // example data
        // { L"1", L"1" },
        // { L"3", L"3", IPLFLAGS_SEPARATOR },
        // { L"4", L"4" },
    };
    return &lines;
}

const view_abstract::sort_modes_t& root_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,        VK_F3 + mods::ctrl },
        { L"Unsorted",  SM_UNSORTED,    VK_F7 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t root_base_view::get_default_settings() const
{
    return { 1, false, 3 };
}

void root_base_view::update_panel_info(OpenPanelInfo *info)
{
    static const wchar_t* titles[] = { L"Name", L"Count" };

    static PanelMode modes[10];

    modes[3].ColumnTypes = L"NON,C0";
    modes[3].ColumnWidths = L"0,6";
    modes[3].ColumnTitles = titles;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    modes[4] = modes[3];

    modes[5] = modes[3];
    modes[5].Flags = PMFLAGS_FULLSCREEN;
    
    modes[8] = modes[3];

    modes[9] = modes[3];

    modes[0].ColumnTypes = L"NON,C0";
    modes[0].ColumnWidths = L"0,6";
    modes[0].ColumnTitles = titles;
    modes[0].StatusColumnTypes = NULL;
    modes[0].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view_abstract::items_t& root_base_view::get_items()
{
    static items_t items; items.clear();
    
    for (const auto &item: menu_items)
    {
        auto totals = get_total(item.id, true);

        wstring entries_count = L"";
        if (totals > 0)
            entries_count = std::format(L"{: >6}", totals);

        items.push_back({
            item.id,
            get_text(item.name_key), get_text(item.descr_key),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            {
                entries_count
            },
            const_cast<root_data_t*>(&item)
        });
    }

    return items;
}

intptr_t root_base_view::select_item(const data_item_t *data)
{
    if (data == nullptr)
        return FALSE;

    for (const auto &mitem: menu_items)
        if (data->id == mitem.id)
        {
            mitem.callback();
            return TRUE;
        }

    return FALSE;
}

bool root_base_view::request_extra_info(const data_item_t* data)
{
    // forcing to request from server and cache
    auto total = get_total(data->id, false);
    return total > 0;
}

root_view::root_view(api_proxy_ptr api):
    root_base_view(api, "root_view", L"", {}, {
        {
            { collection_id },
            MPanelCollectionItemLabel, MPanelCollectionItemDescr,
            std::bind(show_collection, api),
        },
        {
            { browse_id },
            MPanelBrowseItemLabel, MPanelBrowseItemDescr,
            std::bind(show_browse, api),
        },
        {
            { recents_id },
            MPanelRecentsItemLabel, MPanelRecentsItemDescr,
            std::bind(show_recents, api),
        },
    })
    {};

browse_view::browse_view(api_proxy_ptr api):
    root_base_view(
        api, "browse_view", get_text(MPanelBrowseItemLabel),
        std::bind(events::show_root, api), {
        {
            { new_releases_id },
            MPanelNewReleasesItemLabel, MPanelNewReleasesItemDescr,
            std::bind(show_new_releases, api)
        },
        {
            { albums_featuring_likes_id },
            MPanelFeaturingAlbumsItemLabel, MPanelFeaturingAlbumsItemDescr,
            std::bind(show_featuring_albums, api)
        },
        {
            { artists_featuring_likes_id },
            MPanelFeaturingArtistsItemLabel, MPanelFeaturingArtistsItemDescr,
            std::bind(show_featuring_artists, api)
        },
    })
    {}

} // namespace ui
} // namespace spotifar