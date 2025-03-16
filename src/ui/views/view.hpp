#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"
#include "spotify/items.hpp"

namespace spotifar { namespace ui {

class view
{
public:
    struct sort_mode_t
    {
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
        std::vector<wstring> custom_column_data;
        spotify::data_item *user_data;
    };

    typedef std::vector<sort_mode_t> sort_modes_t;
    typedef std::vector<item_t> items_t;
    typedef std::unordered_map<FarKey, wstring> key_bar_info_t;
    typedef std::vector<InfoPanelLine> info_lines_t;

public:
    view(const string &uid);
    virtual ~view() {}

    // a panel's interface to the view data

    /// a helper event from outside, its called right after the items
    /// are populated on the panel
    auto on_items_updated() -> void;
    auto compare_items(const CompareInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto select_item(const SetDirectoryInfo *info) -> intptr_t;
    auto request_extra_info(const PluginPanelItem *item) -> intptr_t;
    auto get_item_idx(const string &item_id) -> size_t;
    auto get_settings() const -> config::settings::view_t*;
    auto select_sort_mode(int sort_mode_idx) -> void;

    virtual auto get_sort_modes() const -> const sort_modes_t& = 0;
    virtual auto get_dir_name() const -> const wstring& = 0;
    virtual auto get_items() -> const items_t* { return nullptr; }
    virtual auto get_key_bar_info() -> const key_bar_info_t* { return nullptr; }
    virtual auto get_info_lines() -> const info_lines_t* { return nullptr; }
    virtual auto update_panel_info(OpenPanelInfo *info) -> void {}
protected:
    /// @brief A helper function to unpack user data from the far items
    static auto unpack_user_data(const UserDataItem &user_data) -> const spotify::data_item*;

    // derived classes' interface to the internal view mechanisms
    virtual auto request_extra_info(const spotify::data_item *data) -> bool { return false; }
    virtual auto select_item(const spotify::data_item *data) -> intptr_t { return FALSE; }
    virtual auto process_key_input(int combined_key) -> intptr_t { return FALSE; }
    virtual auto get_default_settings() const -> config::settings::view_t = 0;
    virtual auto compare_items(const sort_mode_t &modes, const spotify::data_item *data1,
        const spotify::data_item *data2) -> intptr_t { return -2; }
private:
    bool is_first_init = true; // data is ready flag
    sort_modes_t sort_modes;
    config::settings::view_t *settings;
    string uid;
};

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467