#include "root.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using PM = view::panel_mode_t;
using utils::far3::get_text;
using namespace events;

static const view::panel_mode_t::column_t
    Name        { L"NON",   L"Name",        L"0" },
    NameFixed   { L"NON",   L"Name",        L"30" },
    Descr       { L"Z",     L"Description", L"0" };

//-------------------------------------------------------------------------------------------------------------
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
                if (auto *user_data = unpack_user_data(item->UserData); user_data->id == root_view::collection_id)
                {
                    log::global->info("Starting collectiong playback");
                    api->start_playback("spotify:collection");

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

const view::panel_modes_t* root_base_view::get_panel_modes() const
{
    // TODO: columns are being copied, consdider some other ways
    static panel_modes_t modes{
        /* 0 */ PM::dummy(),
        /* 1 */ PM::dummy(),
        /* 2 */ PM::dummy(),
        /* 3 */ PM({ Name }),
        /* 4 */ PM({ NameFixed, Descr }),
        /* 5 */ PM({ NameFixed, Descr }, true),
        /* 6 */ PM::dummy(3),
        /* 7 */ PM::dummy(3),
        /* 8 */ PM::dummy(3),
        /* 9 */ PM::dummy(3),
    };
    
    modes.update(); // TODO: I think it could be emitted
    
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
            { recently_saved_id },
            MPanelRecentlySavedLabel, MPanelRecentlySavedDescr,
            std::bind(show_recently_saved, api)
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