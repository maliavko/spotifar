#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"
#include "spotify/items.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief An abstract class for holding a currently viewable panel's data and
/// business logic. By design, a user can travers through diffrent kind of 
/// Spotify collections, each of them has a different key-features inside.
/// To separate all of them from each other at any moment the panel creates and holds
/// one view and change it by request of the user
class view_abstract
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
        std::vector<wstring> columns_data;
        data_item_t *user_data;
        bool is_selected = false;
    };

    using sort_modes_t = std::vector<sort_mode_t>;
    using items_t = std::vector<item_t>;
    using key_bar_info_t = std::unordered_map<FarKey, wstring>;
    using info_lines_t = std::vector<InfoPanelLine>;
    using return_callback_t = std::function<void(void)>;

public:
    view_abstract(const string &uid, const wstring &title, return_callback_t callback);
    virtual ~view_abstract() {}

    // a public interface for a panel operations on the view

    /// @brief a helper event from outside, its called right after the items
    /// are populated on the panel
    auto on_items_updated() -> void;
    auto compare_items(const CompareInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto select_item(const SetDirectoryInfo *info) -> intptr_t;
    auto request_extra_info(const PluginPanelItem *item) -> intptr_t;
    auto get_item_idx(const string &item_id) -> size_t;
    auto get_settings() const -> config::settings::view_t*;
    auto select_sort_mode(int sort_mode_idx) -> void;
    auto get_return_callback() const -> const return_callback_t& { return return_callback; }

    virtual auto get_dir_name() const -> const wstring& { return title; }
    virtual auto get_sort_modes() const -> const sort_modes_t& = 0;
    virtual auto get_items() -> const items_t* { return nullptr; }
    virtual auto get_key_bar_info() -> const key_bar_info_t* { return nullptr; }
    virtual auto get_info_lines() -> const info_lines_t* { return nullptr; }
    virtual void update_panel_info(OpenPanelInfo *info) {}
protected:
    /// @brief A helper function to unpack user data from the far items
    static auto unpack_user_data(const UserDataItem &user_data) -> const data_item_t*;

    // derived classes' interface to the internal view mechanisms
    virtual auto request_extra_info(const data_item_t *data) -> bool { return false; }
    virtual auto select_item(const data_item_t *data) -> intptr_t { return FALSE; }
    virtual auto process_key_input(int combined_key) -> intptr_t { return FALSE; }
    virtual auto get_default_settings() const -> config::settings::view_t = 0;
    virtual auto compare_items(const sort_mode_t &modes, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t { return -2; }
private:
    return_callback_t return_callback;
    bool is_first_init = true; // data-is-set flag
    sort_modes_t sort_modes;
    config::settings::view_t *settings;
    string uid;
    wstring title;
};

using view_ptr = std::shared_ptr<view_abstract>;

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467