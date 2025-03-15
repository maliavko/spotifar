#include "root.hpp"
#include "ui/events.hpp"
#include "spotify/requests.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    collection_view_id = "collection",
    browse_view_id = "browse",
    recents_view_id = "recents";

root_view::root_view(api_abstract *api):
    view("root_view"),
    api_proxy(api)
{
}

const wstring& root_view::get_dir_name() const
{
    // should be empty, so Far closes plugin in case of hitting ".."
    static wstring cur_dir(L"");
    return cur_dir;
}

const view::key_bar_info_t* root_view::get_key_bar_info()
{
    // TODO: test data
    static key_bar_info_t key_bar{
        { { VK_F4, 0 }, get_text(MKeyBarF4) },
    };

    return &key_bar;
}

const view::info_lines_t* root_view::get_info_lines()
{
    // TODO: test data
    static info_lines_t lines{
        { L"1", L"1" },
        { L"3", L"3", IPLFLAGS_SEPARATOR },
        { L"4", L"4" },
    };
    return &lines;
}

const view::sort_modes_t& root_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",      SM_NAME,        VK_F3 + mods::ctrl },
        { L"Unsorted",  SM_UNSORTED,    VK_F7 + mods::ctrl },
    };
    return modes;
}

config::settings::view_t root_view::get_default_settings() const
{
    return { 1, false, 3 };
}

void root_view::update_panel_info(OpenPanelInfo *info)
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

const view::items_t* root_view::get_items()
{
    static items_t items;
    
    items.assign({
        pack_menu_item(collection_view_id, MPanelCollectionItemLabel, MPanelCollectionItemDescr),
        pack_menu_item(browse_view_id, MPanelBrowseItemLabel, MPanelBrowseItemDescr),
        pack_menu_item(recents_view_id, MPanelRecentsItemLabel, MPanelRecentsItemDescr),
    });

    return &items;
}

intptr_t root_view::select_item(const user_data_t *data)
{
    if (data == nullptr)
        return FALSE;

    if (data->id == collection_view_id)
    {
        ui::events::show_collection_view(api_proxy);
        return TRUE;
    }
    
    if (data->id == browse_view_id)
    {
        ui::events::show_browse_view(api_proxy);
        return TRUE;
    }
    
    if (data->id == recents_view_id)
    {
        ui::events::show_recents_view(api_proxy);
        return TRUE;
    }

    return FALSE;
}

view::items_t::value_type root_view::pack_menu_item(const string &id, int label_id, int descr_id)
{
    return {
        id, get_text(label_id), get_text(descr_id),
        FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL, {},
        new user_data_t{ id }, view::free_user_data<view::user_data_t>
    };
}

} // namespace ui
} // namespace spotifar