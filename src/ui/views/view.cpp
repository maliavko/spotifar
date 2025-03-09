#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {
    
view::view(const string &uid):
    uid(uid)
{
}

void view::init()
{
    settings = config::get_panel_settings(uid, get_default_settings());

    const auto sm = get_sort_modes();
    if (sm != nullptr)
        sort_modes = *sm;
}

void view::on_items_updated()
{
    if (is_first_initialization)
    {
        is_first_initialization = false;
        
        if (sort_modes.size() > settings->sort_mode_idx)
            utils::far3::panels::set_sort_mode(PANEL_ACTIVE,
                sort_modes[settings->sort_mode_idx].far_sort_mode, settings->is_descending);
    }
}

FARPANELITEMFREECALLBACK view::get_free_user_data_callback()
{
    return user_data_t::free;
}

const view::user_data_t* view::unpack_user_data(const UserDataItem &user_data)
{
    if (user_data.Data != nullptr)
        return reinterpret_cast<const view::user_data_t*>(user_data.Data);
    return nullptr;
}

void WINAPI view::user_data_t::free(void *const user_data, const FarPanelItemFreeInfo *const info)
{
    delete reinterpret_cast<const user_data_t*>(user_data);
}

intptr_t view::process_key_input(int combined_key)
{
    for (int idx = 0; idx < sort_modes.size(); idx++)
    {
        const auto &smode = sort_modes[idx];
        if (combined_key == smode.combined_key)
        {
            if (idx == settings->sort_mode_idx)
                settings->is_descending = !settings->is_descending;
            else
                settings->sort_mode_idx = idx;
            utils::far3::panels::set_sort_mode(PANEL_ACTIVE, smode.far_sort_mode, settings->is_descending);
            return TRUE;
        }
    }
    return FALSE;
}

view::item_t::item_t(const string &id, const wstring &name, const wstring &descr,
                uintptr_t attrs, size_t size, const std::vector<wstring> &custom_column_data,
                user_data_t *user_data):
    id(id),
    name(utils::strip_invalid_filename_chars(name)),
    description(descr),
    file_attrs(attrs),
    size(size),
    custom_column_data(custom_column_data),
    user_data(user_data)
{
}

} // namespace ui
} // namespace spotifar