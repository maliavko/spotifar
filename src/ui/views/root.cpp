#include "root.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

static const string
    collection_id = "collection",
    browse_id = "browse",
    recents_id = "recents";

struct root_data_t: public data_item_t
{
    int name_key, descr_key;
};

static std::vector<root_data_t> menu_items{
    { { collection_id }, MPanelCollectionItemLabel, MPanelCollectionItemDescr },
    { { browse_id }, MPanelBrowseItemLabel, MPanelBrowseItemDescr },
    { { recents_id }, MPanelRecentsItemLabel, MPanelRecentsItemDescr },
};

root_view::root_view(api_abstract *api):
    view("root_view", {}),
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
    static items_t items; items.clear();
    
    for (auto &item: menu_items)
    {
        items.push_back({
            item.id,
            get_text(item.name_key), get_text(item.descr_key),
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL, {},
            &item
        });
    }

    return &items;
}

intptr_t root_view::select_item(const data_item_t *data)
{
    if (data == nullptr)
        return FALSE;

    if (data->id == collection_id)
    {
        ui::events::show_collections(api_proxy);
        return TRUE;
    }
    
    if (data->id == browse_id)
    {
        ui::events::show_browse(api_proxy);
        return TRUE;
    }
    
    if (data->id == recents_id)
    {
        ui::events::show_recents(api_proxy);
        return TRUE;
    }

    return FALSE;
}

} // namespace ui
} // namespace spotifar