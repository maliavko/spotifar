#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace ui {

class view
{
public:
    /// @brief a proxy item, used for passing data from controllers to FAR panels api
    struct item_t
    {
        string id;
        wstring name;
        wstring description;
        uintptr_t file_attrs;
        size_t size;
        std::vector<wstring> custom_column_data;

        item_t(const string &id, const wstring &name, const wstring &descr,
            uintptr_t attrs, size_t size = 0, const std::vector<wstring> &columns_data = {});
    };

    typedef std::vector<item_t> items_t;
    typedef std::unordered_map<FarKey, wstring> key_bar_info_t;
    typedef std::vector<InfoPanelLine> info_lines_t;

public:
    view() {}
    virtual ~view() {}

    virtual auto get_dir_name() const -> const wchar_t* = 0;
    virtual auto get_title() const -> const wchar_t* = 0;
    
    virtual auto get_items() -> const items_t* { return nullptr; }
    virtual auto get_key_bar_info() -> const key_bar_info_t* { return nullptr; }
    virtual auto get_info_lines() -> const info_lines_t* { return nullptr; }

    virtual auto select_item(const string &item_id) -> intptr_t { return FALSE; }
    virtual auto get_item_idx(const string &item_id) -> size_t { return 0; }
    virtual auto process_input(const ProcessPanelInputInfo *info) -> intptr_t { return FALSE; }
    virtual auto update_panel_info(OpenPanelInfo *info) -> void {}
    virtual auto request_extra_info(const string &item_id) -> bool { return false; }
protected:
    const wstring name;
};

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467