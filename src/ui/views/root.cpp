#include "root.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
using namespace events;

//-------------------------------------------------------------------------------------------------------------
const view::key_bar_info_t* root_base_view::get_key_bar_info()
{
    static key_bar_info_t key_bar{
        // example data
        // { { VK_F4, 0 }, get_text(MKeyBarF4) },
    };

    return &key_bar;
}

const view::info_lines_t* root_base_view::get_info_lines()
{
    static info_lines_t lines{
        // example data
        // { L"1", L"1" },
        // { L"3", L"3", IPLFLAGS_SEPARATOR },
        // { L"4", L"4" },
    };
    return &lines;
}

const view::sort_modes_t& root_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,        { VK_F3, LEFT_CTRL_PRESSED } },
        { L"Unsorted",  SM_UNSORTED,    { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

config::settings::view_t root_base_view::get_default_settings() const
{
    return { 1, false, 3 };
}

void root_base_view::update_panel_info(OpenPanelInfo *info)
{

    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name" };
    modes[3].ColumnTypes = L"NON";
    modes[3].ColumnWidths = L"0";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    static const wchar_t* titles_4[] = { L"Name", L"Description" };
    modes[4].ColumnTypes = L"NON,Z";
    modes[4].ColumnWidths = L"30,0";
    modes[4].ColumnTitles = titles_4;
    modes[4].StatusColumnTypes = NULL;
    modes[4].StatusColumnWidths = NULL;

    modes[5] = modes[4];
    modes[5].Flags = PMFLAGS_FULLSCREEN;
    
    modes[6] = modes[3];
    modes[7] = modes[3];
    modes[8] = modes[3];
    modes[9] = modes[3];

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

const view::items_t& root_base_view::get_items()
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
    return get_total(data->id, false) > 0;
}


//-------------------------------------------------------------------------------------------------------------
root_view::root_view(HANDLE panel, api_weak_ptr_t api):
    root_base_view(panel, api, L"", {
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
            { recently_played_id },
            MPanelRecentsItemLabel, MPanelRecentsItemDescr,
            std::bind(show_recents, api),
        },
        {
            { playing_queue_id },
            MPanelPlayingQueueItemLabel, MPanelPlayingQueueItemDescr,
            std::bind(show_playing_queue, api),
        },
    })
    {};


//-------------------------------------------------------------------------------------------------------------
browse_view::browse_view(HANDLE panel, api_weak_ptr_t api):
    root_base_view(
        panel, api, get_text(MPanelBrowseItemLabel), {
        {
            { new_releases_id },
            MPanelNewReleasesItemLabel, MPanelNewReleasesItemDescr,
            std::bind(show_new_releases, api)
        },
        {
            { recently_liked_tracks_id },
            MPanelRecentlyLikedTracksLabel, MPanelRecentlyLikedTracksDescr,
            std::bind(show_recently_liked_tracks, api)
        },
        {
            { recently_saved_albums_id },
            MPanelRecentlySavedAlbumsLabel, MPanelRecentlySavedAlbumsDescr,
            std::bind(show_recently_saved_albums, api)
        },
        {
            { user_top_items_id },
            MPanelUserTopTracksLabel, MPanelUserTopTracksDescr,
            std::bind(show_user_top_items, api)
        },
    })
    {}

} // namespace ui
} // namespace spotifar