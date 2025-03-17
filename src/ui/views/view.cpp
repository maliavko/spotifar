#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

namespace panels = utils::far3::panels;

view::view(const string &uid, return_callback_t callback):
    uid(uid),
    return_callback(callback)
{
}

config::settings::view_t* view::get_settings() const
{
    return config::get_panel_settings(uid, get_default_settings());
}

void view::select_sort_mode(int idx)
{
    if (idx > sort_modes.size())
    {
        utils::log::global->error("Given sort mode index is out of range, index {}, "
            "view uid {}, modes count {}", idx, uid, sort_modes.size());
        return;
    }

    const auto &sm = sort_modes[idx];

    if (idx == settings->sort_mode_idx)
        // if the sort mode index is the same as the current one - we invert
        // the sorting direction
        settings->is_descending = !settings->is_descending;
    else
        // otherwise, just change the sort mode
        settings->sort_mode_idx = idx;
    
    panels::set_sort_mode(PANEL_ACTIVE, sm.far_sort_mode, settings->is_descending);
}

void view::on_items_updated()
{
    if (is_first_init)
    {
        is_first_init = false;

        settings = get_settings();
        sort_modes = get_sort_modes();
        
        if (sort_modes.size() > settings->sort_mode_idx)
            panels::set_sort_mode(PANEL_ACTIVE,
                sort_modes[settings->sort_mode_idx].far_sort_mode, settings->is_descending);
    }
}

const data_item_t* view::unpack_user_data(const UserDataItem &user_data)
{
    if (user_data.Data != nullptr)
        return reinterpret_cast<const data_item_t*>(user_data.Data);
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
                select_sort_mode(idx);
                return TRUE;
            }
        }

        return process_key_input(key);
    }
    return FALSE;
}

intptr_t view::select_item(const SetDirectoryInfo *info)
{
    if (info->UserData.Data == nullptr)
    {
        if (return_callback)
            return_callback();
        return TRUE;
    }

    return select_item(unpack_user_data(info->UserData));
}

intptr_t view::request_extra_info(const PluginPanelItem *data)
{
    return request_extra_info(unpack_user_data(data->UserData));
}

size_t view::get_item_idx(const string &item_id)
{
    const auto &items = panels::get_items(PANEL_ACTIVE);
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