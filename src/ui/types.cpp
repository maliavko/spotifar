#include "ui/types.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

int sort_mode_t::get_combined_key() const
{
    return utils::keys::make_combined(far_key);
}

panel_mode_t::panel_mode_t(std::vector<const column_t*> &&cols, bool is_wide)
{
    columns = std::move(cols);

    if (is_wide)
        flags |= PMFLAGS_FULLSCREEN;
    
    rebuild();
}

panel_mode_t::panel_mode_t(int copy_of_idx, bool is_wide): copy_of_idx(copy_of_idx)
{
    if (is_wide)
        flags |= PMFLAGS_FULLSCREEN;
}

void panel_mode_t::insert_column(const column_t *col, size_t idx, bool replace)
{
    if (idx > columns.size())
    {
        log::global->error("Trying to insert a new column for panel mode into wrong index");
        return;
    }

    auto it = columns.begin();
    std::advance(it, idx);
    
    if (replace)
    {
        it = columns.erase(it);
    }
    
    columns.insert(it, col);
    rebuild();
}

void panel_mode_t::rebuild()
{
    titles.clear();
    std::vector<const wchar_t*> all_types, all_widths;

    for (const auto &column: columns)
    {
        titles.push_back(column->title);
        all_types.push_back(column->uid);
        all_widths.push_back(column->width);
    }

    if (columns.size() > 0)
    {
        types = utils::string_join(all_types, L",");
        widths = utils::string_join(all_widths, L",");
    }
}

panel_modes_t::panel_modes_t(std::initializer_list<panel_mode_t> il): base_type_t(il)
{
    rebuild();
}

void panel_modes_t::rebuild()
{
    for (size_t i = 0; i < MODES_COUNT; i++)
    {
        auto &mode = at(i);

        // if the mode is empty, we check whether we need to reference to another one
        if (mode.is_empty() && mode.copy_of_idx >= 0)
            mode = at(mode.copy_of_idx);

        // filling up the Far mode struct in case we eventually found a valid object
        if (!mode.is_empty())
        {
            auto &far_mode = modes[i];

            far_mode.ColumnTypes = mode.types.c_str();
            far_mode.ColumnWidths = mode.widths.c_str();
            far_mode.ColumnTitles = &mode.titles[0];
            far_mode.Flags = mode.flags;
            far_mode.StatusColumnTypes = NULL;
            far_mode.StatusColumnWidths = NULL;
        }
    }
}

} // namespace ui
} // namespace spotifar