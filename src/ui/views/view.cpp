#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {
    
view::view(const string &uid):
    uid(uid)
{
}

config::settings::view_t* view::get_settings() const
{
    return config::get_panel_settings(uid, get_default_settings());
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