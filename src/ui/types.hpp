#ifndef TYPES_HPP_3D4B2974_24D0_48E9_B3A3_4BD005CC78D5
#define TYPES_HPP_3D4B2974_24D0_48E9_B3A3_4BD005CC78D5
#pragma once

#include "stdafx.h"

namespace spotifar { namespace ui {

struct sort_mode_t
{
    wstring name;
    OPENPANELINFO_SORTMODES far_sort_mode;
    FarKey far_key;

    int get_combined_key() const;
};

/// @brief a proxy item, used for passing data from controllers to FAR panels api
struct item_t
{
    string id;
    wstring name;
    wstring description;
    uintptr_t file_attrs;
    std::vector<wstring> columns_data; 
    spotify::data_item_t *user_data;
    bool is_selected = false;
};

struct panel_mode_t
{
    struct column_t
    {
        const wchar_t *uid;
        const wchar_t *title;
        const wchar_t *width;
    };

    /// @brief contains the Far PanelMode struct compatible columns types string
    wstring types = L"";
    
    /// @brief contains the Far PanelMode struct compatible columns widths string
    wstring widths = L"";
    
    /// @brief contains the Far PanelMode struct compatible columns titles array
    std::vector<const wchar_t*> titles;

    int flags = PMFLAGS_NONE;
    int copy_of_idx = -1;

    /// @brief A default constructor, taking a vector of columns of the panel view mode.
    /// In the end of the initialization the method `rebuild` is called
    /// @param is_wide whether the panel mode should be wide (occupy two panels)
    panel_mode_t(std::vector<const column_t*> &&cols, bool is_wide = false);

    /// @brief An aux constructor, which initializes a dummy panel mode object, copying
    /// the actual data from the mode of `copy_of_idx` index. Can copy the mode and
    /// override its width behaviour by specifying `is_wide` flag
    panel_mode_t(int copy_of_idx, bool is_wide = false);

    /// @brief The mode does not have any columns
    bool is_empty() const { return columns.size() == 0; }

    /// @brief Inserts a given column `col` to the mode at the specified position `idx`
    void insert_column(const column_t *col, size_t idx);

    /// @brief Rebuild the public members according to the latest columns
    /// data provided.
    void rebuild();

    static panel_mode_t dummy(int copy_of_idx = -1, bool is_wide = false)
    {
        return panel_mode_t(copy_of_idx, is_wide);
    }
private:
    std::vector<const column_t*> columns;
};

/// @brief Class-holder the panel view modes as an array. Helps
/// to build the modes, prepare the data, compatible for Far PanelMode[]
class panel_modes_t: std::vector<panel_mode_t>
{
    using base_type_t = std::vector<panel_mode_t>;
public:
    static const size_t MODES_COUNT = 10;
    
    using vector::size;
    using vector::at;
    using vector::operator[];

    panel_modes_t(): base_type_t{} {}
    panel_modes_t(std::initializer_list<panel_mode_t> il);

    const PanelMode* get_modes() const { return modes; }

    /// @brief Rebuilds the panel modes, according to the latest
    /// changed data in the containers
    void rebuild();
private:
    PanelMode modes[MODES_COUNT];
};

class view;

using sort_modes_t = std::vector<sort_mode_t>;
using items_t = std::vector<item_t>;
using key_bar_info_t = std::unordered_map<FarKey, const wchar_t*>;
using info_lines_t = std::vector<InfoPanelLine>;
using return_callback_t = std::function<void(void)>;
using view_ptr_t = std::shared_ptr<view>;

} // namespace ui
} // namescape spotifar

#endif // TYPES_HPP_3D4B2974_24D0_48E9_B3A3_4BD005CC78D5