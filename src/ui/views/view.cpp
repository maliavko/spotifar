#include "view.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

    view_item::view_item(const string &id, const wstring &name, const wstring &descr,
                         uintptr_t attrs, size_t duration):
        id(id),
        name(utils::strip_invalid_filename_chars(name)),
        description(descr),
        file_attrs(attrs),
        duration(duration)
    {
    }

} // namespace ui
} // namespace spotifar