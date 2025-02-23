#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace ui {

class view
{
public:
    struct item_t
    {
        string id;
        wstring name;
        wstring description;
        uintptr_t file_attrs;
        std::vector<wstring> custom_column_data;

        item_t(const string &id, const wstring &name, const wstring &descr,
            uintptr_t attrs, const std::vector<wstring> &columns_data = {});
    };

    typedef std::vector<item_t> items_t;
    typedef std::unordered_map<FarKey, wstring> key_bar_info_t;
    typedef std::vector<InfoPanelLine> info_lines_t;

public:
    view() {}
    virtual ~view() {}

    virtual const wchar_t* get_dir_name() const = 0;
    virtual const wchar_t* get_title() const = 0;
    
    virtual items_t get_items() { return {}; }
    virtual auto get_key_bar_info() -> const key_bar_info_t* { return nullptr; }
    virtual auto get_info_lines() -> const info_lines_t* { return nullptr; }

    virtual intptr_t select_item(const string &id) { return FALSE; }
    virtual size_t get_item_idx(const string &item_id) { return 0; }
    virtual intptr_t process_input(const ProcessPanelInputInfo *info) { return FALSE; }
    virtual void update_panel_info(OpenPanelInfo *info) {}

protected:
    const wstring name;
};

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467