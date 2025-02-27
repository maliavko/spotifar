#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

    view::item_t::item_t(const string &id, const wstring &name, const wstring &descr,
                   uintptr_t attrs, size_t size, const std::vector<wstring> &custom_column_data):
        id(id),
        name(utils::strip_invalid_filename_chars(name)),
        description(descr),
        file_attrs(attrs),
        size(size),
        custom_column_data(custom_column_data)
    {
    }

} // namespace ui
} // namespace spotifar