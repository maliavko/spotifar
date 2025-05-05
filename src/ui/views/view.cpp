#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;
namespace panels = far3::panels;
namespace keys = utils::keys;

view_abstract::view_abstract(const string &uid, const wstring &title, return_callback_t callback):
    uid(uid),
    title(title),
    return_callback(callback)
{
}

config::settings::view_t* view_abstract::get_settings() const
{
    return config::get_panel_settings(uid, get_default_settings());
}

void view_abstract::select_sort_mode(int idx)
{
    if (idx > sort_modes.size())
    {
        log::global->error("Given sort mode index is out of range, index {}, "
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

void view_abstract::on_items_updated()
{
    if (is_first_init)
    {
        is_first_init = false;

        settings = get_settings();
        sort_modes = get_sort_modes();
        
        // settings a stored sort mode
        if (sort_modes.size() > settings->sort_mode_idx)
            panels::set_sort_mode(PANEL_ACTIVE,
                sort_modes[settings->sort_mode_idx].far_sort_mode, settings->is_descending);

        // settings a stored view mode
        panels::set_view_mode(PANEL_ACTIVE, settings->view_mode);
    }
}

const data_item_t* view_abstract::unpack_user_data(const UserDataItem &user_data)
{
    if (user_data.Data != nullptr)
        return reinterpret_cast<const data_item_t*>(user_data.Data);
    return nullptr;
}

intptr_t view_abstract::compare_items(const CompareInfo *info)
{
    if (sort_modes.size() > settings->sort_mode_idx)
        return compare_items(
            sort_modes[settings->sort_mode_idx],
            unpack_user_data(info->Item1->UserData),
            unpack_user_data(info->Item2->UserData)
        );
    return -2;
}

intptr_t view_abstract::process_input(const ProcessPanelInputInfo *info)
{
    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        int key = keys::make_combined(key_event);
        
        for (int idx = 0; idx < sort_modes.size(); idx++)
        {
            const auto &smode = sort_modes[idx];
            if (key == smode.combined_key)
            {
                select_sort_mode(idx);
                return TRUE;
            }
        }
        
        // all the sorting hotkeys Ctrl+(F3...F12) are blocked, due to custom plugin implementation
        for (int key_code = VK_F3; key_code <= VK_F12; key_code++)
            if (key == key_code + keys::mods::ctrl)
                return TRUE;
        
        // view mode hotkeys Ctrl+(1...0) pre-handling: saving the index into
        // specific view persistent settings to recover later
        for (int key_code = keys::key_0; key_code <= keys::key_9; key_code++)
            if (key == key_code + keys::mods::ctrl)
            {
                settings->view_mode = key_code - keys::key_0;
                return TRUE;
            }

        return process_key_input(key);
    }
    return FALSE;
}

intptr_t view_abstract::select_item(const SetDirectoryInfo *info)
{
    if (info->UserData.Data == nullptr)
    {
        if (return_callback)
            return_callback();
        return TRUE;
    }

    return select_item(unpack_user_data(info->UserData));
}

intptr_t view_abstract::request_extra_info(const PluginPanelItem *data)
{
    return request_extra_info(unpack_user_data(data->UserData));
}

size_t view_abstract::get_item_idx(const string &item_id)
{
    const auto &items = panels::get_items(PANEL_ACTIVE);
    for (size_t idx = 1; idx < items.size(); idx++) // zero index is ".." folder
    {
        auto user_data = unpack_user_data(items[idx]->UserData);
        if (user_data != nullptr && user_data->id == item_id)
           return idx;
    }
    return 0;
}

} // namespace ui
} // namespace spotifar