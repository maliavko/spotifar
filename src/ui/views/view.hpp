#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace ui {

class view
{
public:
    struct user_data_t
    {
        string id;
        wstring name;
        
        static void WINAPI free(void *const user_data,
            const FarPanelItemFreeInfo *const info);
    };

    struct sort_mode_t
    {
        //typedef 
        wstring name;
        OPENPANELINFO_SORTMODES far_sort_mode;
        int combined_key;
    };

    /// @brief a proxy item, used for passing data from controllers to FAR panels api
    struct item_t
    {
        string id;
        wstring name;
        wstring description;
        uintptr_t file_attrs;
        size_t size; // TODO: it seems, it is not used
        std::vector<wstring> custom_column_data;
        user_data_t *user_data;

        item_t(const string &id, const wstring &name, const wstring &descr,
            uintptr_t attrs, size_t size = 0, const std::vector<wstring> &columns_data = {},
            user_data_t *user_data = nullptr);
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
    virtual auto get_item_idx(const string &item_id) -> size_t { return 0; }

    virtual auto select_item(const user_data_t* data) -> intptr_t { return FALSE; }
    virtual auto process_key_input(int combined_key) -> intptr_t;
    virtual auto update_panel_info(OpenPanelInfo *info) -> void {}
    virtual auto request_extra_info(const user_data_t* data) -> bool { return false; }
    virtual auto compare_items(const user_data_t* data1, const user_data_t* data2) -> intptr_t { return -2; }
    virtual auto get_free_user_data_callback() -> FARPANELITEMFREECALLBACK;
    virtual auto unpack_user_data(const UserDataItem &user_data) -> const user_data_t*;
protected:
    const sort_mode_t &get_sort_mode() const { return sort_modes[sort_mode_idx]; }
protected:
    const wstring name;

    std::vector<sort_mode_t> sort_modes;
    size_t sort_mode_idx = 0;
    bool is_descending = false;
};

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467