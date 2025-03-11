#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

view::view(const string &uid):
    uid(uid)
{
}

config::settings::view_t* view::get_settings()
{
    return config::get_panel_settings(uid, get_default_settings());
}

void view::on_items_updated()
{
    if (is_first_init)
    {
        is_first_init = false;

        settings = get_settings();
        sort_modes = get_sort_modes();
        
        if (sort_modes.size() > settings->sort_mode_idx)
            utils::far3::panels::set_sort_mode(PANEL_ACTIVE,
                sort_modes[settings->sort_mode_idx].far_sort_mode, settings->is_descending);
    }
}

const view::user_data_t* view::unpack_user_data(const UserDataItem &user_data)
{
    if (user_data.Data != nullptr)
        return reinterpret_cast<const user_data_t*>(user_data.Data);
    return nullptr;
}

intptr_t view::compare_items(const CompareInfo *info)
{
    if (sort_modes.size() > settings->sort_mode_idx)
        return compare_items(
            sort_modes[settings->sort_mode_idx],
            unpack_user_data(info->Item1->UserData),
            unpack_user_data(info->Item2->UserData)
        );
    return -2;
}

intptr_t view::process_input(const ProcessPanelInputInfo *info)
{
    auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        int key = utils::keys::make_combined(key_event);
        
        for (int idx = 0; idx < sort_modes.size(); idx++)
        {
            const auto &smode = sort_modes[idx];
            if (key == smode.combined_key)
            {
                if (idx == settings->sort_mode_idx)
                    settings->is_descending = !settings->is_descending;
                else
                    settings->sort_mode_idx = idx;
                utils::far3::panels::set_sort_mode(PANEL_ACTIVE, smode.far_sort_mode, settings->is_descending);
                return TRUE;
            }
        }

        return process_key_input(key);
    }
    return FALSE;
}

intptr_t view::select_item(const SetDirectoryInfo *info)
{
    return select_item(unpack_user_data(info->UserData));
}

intptr_t view::request_extra_info(const PluginPanelItem *data)
{
    return request_extra_info(unpack_user_data(data->UserData));
}

size_t view::get_item_idx(const string &item_id)
{
    const auto &items = utils::far3::panels::get_items(PANEL_ACTIVE);
    for (size_t idx = 1; idx <= items.size(); idx++)
    {
        auto user_data = unpack_user_data(items[idx]->UserData);
        if (user_data != nullptr && user_data->id == item_id)
            return idx;
    }
    return 0;
}

} // namespace ui
} // namespace spotifar