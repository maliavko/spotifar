#include "ui/types.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

int sort_mode_t::get_combined_key() const
{
    return utils::keys::make_combined(far_key);
}

} // namespace ui
} // namespace spotifar