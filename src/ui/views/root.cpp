#include "root.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using PM = panel_mode_t;
using utils::far3::get_text;
using namespace events;

//-------------------------------------------------------------------------------------------------------------
const info_lines_t* root_base_view::get_info_lines()
{
    static info_lines_t lines{
        // example data
        // { L"1", L"1" },
        // { L"3", L"3", IPLFLAGS_SEPARATOR },
        // { L"4", L"4" },
    };
    return &lines;
}

const sort_modes_t& root_base_view::get_sort_modes() const
{
    static sort_modes_t modes = {
        { get_text(MSortBarName),      SM_NAME,        { VK_F3, LEFT_CTRL_PRESSED } },
        { get_text(MSortBarUnsorted),  SM_UNSORTED,    { VK_F7, LEFT_CTRL_PRESSED } },
    };
    return modes;
}

config::settings::view_t root_base_view::get_default_settings() const
{
    // sort mode - SM_UNSORTED; ascending; view mode - 4
    return { 1, false, 4 };
}

const items_t& root_base_view::get_items()
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
    if (data == nullptr) return FALSE;

    for (const auto &mitem: menu_items)
        if (data->id == mitem.id)
        {
            mitem.callback();
            return TRUE;
        }

    return FALSE;
}

intptr_t root_base_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_F4:
        {
            auto item = utils::far3::panels::get_current_item(get_panel_handle());
            if (auto api = api_proxy.lock(); item && api)
            {
                auto *user_data = unpack_user_data(item->UserData);
                if (user_data && user_data->id == root_view::collection_id)
                {
                    log::global->info("Starting collection playback");
                    api->start_playback(COLLECTION_URI);

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

const panel_modes_t* root_base_view::get_panel_modes() const
{
    static const panel_mode_t::column_t
        Name        { L"NON",   get_text(MSortColName),     L"0" },
        NameFixed   { L"NON",   get_text(MSortColName),     L"30" },
        Descr       { L"Z",     get_text(MSortColDescr),    L"0" };
    
    static panel_modes_t modes{
        /* 0 */ PM::dummy(),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ &Name }),
        /* 4 */ PM({ &NameFixed, &Descr }),
        /* 5 */ PM({ &NameFixed, &Descr }, true),
        /* 6 */ PM::dummy(3),
        /* 7 */ PM::dummy(5),
        /* 8 */ PM::dummy(3),
        /* 9 */ PM::dummy(3),
    };
    
    return &modes;
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
            MPanelCollection, MPanelCollectionDescr,
            [api] { show_collection(api); },
        },
        {
            { browse_id },
            MPanelBrowse, MPanelBrowseDescr,
            [api] { show_browse(api); },
        },
        {
            { playing_queue_id },
            MPanelPlayingQueue, MPanelPlayingQueueDescr,
            [api] { show_playing_queue(api); },
        },
    })
    {};


//-------------------------------------------------------------------------------------------------------------
browse_view::browse_view(HANDLE panel, api_weak_ptr_t api):
    root_base_view(
        panel, api, get_text(MPanelBrowse), {
        {
            { new_releases_id },
            MPanelNewReleases, MPanelNewReleasesDescr,
            [api] { show_new_releases(api); },
        },
        {
            { recently_saved_id },
            MPanelRecentlySaved, MPanelRecentlySavedDescr,
            [api] { show_recently_saved(api); },
        },
        {
            { recently_played_id },
            MPanelRecents, MPanelRecentsDescr,
            [api] { show_recents(api); },
        },
        {
            { user_top_items_id },
            MPanelUserTopItems, MPanelUserTopItemsDescr,
            [api] { show_user_top_items(api); },
        },
    })
    {}

} // namespace ui
} // namespace spotifar