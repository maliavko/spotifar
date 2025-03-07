#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

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
    
    FARPANELITEMFREECALLBACK view::get_free_user_data_callback()
    {
        return user_data_t::free;
    }
    
    intptr_t view::process_key_input(int combined_key)
    {
        for (size_t idx = 0; idx < sort_modes.size(); idx++)
        {
            const auto &smode = sort_modes[idx];
            if (combined_key == smode.combined_key)
            {
                if (idx == sort_mode_idx)
                    is_descending = !is_descending;
                else
                    sort_mode_idx = idx;
                utils::far3::panels::set_sort_mode(PANEL_ACTIVE, smode.far_sort_mode, is_descending);
                return TRUE;
            }
        }
        return FALSE;
    }

    void WINAPI view::user_data_t::free(void *const user_data, const FarPanelItemFreeInfo *const info)
    {
        delete reinterpret_cast<const user_data_t*>(user_data);
    }
    
    const view::user_data_t* view::unpack_user_data(const UserDataItem &user_data)
    {
        if (user_data.Data != nullptr)
            return reinterpret_cast<const view::user_data_t*>(user_data.Data);
        return nullptr;
    }

} // namespace ui
} // namespace spotifar