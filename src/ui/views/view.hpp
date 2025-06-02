#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"
#include "spotify/items.hpp"
#include "spotify/common.hpp"

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
        FarKey far_key;

        int get_combined_key() const { return utils::keys::make_combined(far_key); }
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
    using key_bar_info_t = std::unordered_map<FarKey, const wchar_t*>;
    using info_lines_t = std::vector<InfoPanelLine>;
    using return_callback_t = std::function<void(void)>;
public:
    /// @param panel a real param handle, PANEL_PASSIVE dose not work
    /// @param title a label used to be shown on top of the panel
    view_abstract(HANDLE panel, const wstring &title):
        panel(panel), title(title)
        {}
    
    virtual ~view_abstract() {}

    // a public interface, exposed to the panel class

    /// @brief a helper event from outside, its called right after the items
    /// are populated on the panel
    void on_items_updated();
    auto compare_items(const CompareInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto select_item(const SetDirectoryInfo *info) -> intptr_t;

    /// @brief Called when user hits F3 key on the panel, returns an
    /// additional info for the panel item under cursor
    auto request_extra_info(const PluginPanelItem *item) -> intptr_t;

    /// @brief Searches for the `item_id` item on the panel and
    /// return its index or 0
    auto get_item_idx(const string &item_id) -> size_t;
    auto get_settings() const -> config::settings::view_t*;
    void select_sort_mode(int sort_mode_idx);

    /// @param callback a handler, which is being called when ".." is hit on the panel
    auto set_return_callback(return_callback_t callback) { return_callback = callback; }
    
    /// @return A handler which is being called when ".." is hit on the panel
    auto get_return_callback() const -> const return_callback_t& { return return_callback; }

    virtual auto get_sort_modes() const -> const sort_modes_t& = 0;
    virtual auto get_items() -> const items_t& = 0;
    virtual auto get_title() const -> const wstring& { return title; }
    virtual auto get_dir_name() const -> const wstring& { return get_title(); }
    virtual auto get_key_bar_info() -> const key_bar_info_t* { return nullptr; }
    virtual auto get_info_lines() -> const info_lines_t* { return nullptr; }
    virtual void update_panel_info(OpenPanelInfo *info) {}

    /// @brief A handler used for showing a filtering dialog, by default show nothing.
    /// Called when user hits Ctrl+I combination on the panels
    virtual void show_filters_dialog() {}
protected:
    /// @brief A helper function to unpack user data from the far items
    static auto unpack_user_data(const UserDataItem &user_data) -> const data_item_t*;

    /// @brief Returns a unique view string id, used in caching
    string get_uid() const { return typeid(*this).name(); }

    /// @brief Returns a panel handle, the view is associsted with
    HANDLE get_panel_handle() const { return panel; }

    // derived classes' interface to the internal view mechanisms
    virtual auto get_default_settings() const -> config::settings::view_t = 0;
    virtual bool request_extra_info(const data_item_t *data) { return false; }
    virtual auto select_item(const data_item_t *data) -> intptr_t { return FALSE; }
    virtual auto process_key_input(int combined_key) -> intptr_t { return FALSE; }
    virtual auto compare_items(const sort_mode_t &modes, const data_item_t *data1, const data_item_t *data2) -> intptr_t { return -2; }
private:
    bool is_first_init = true; // data-is-set flag
    return_callback_t return_callback;
    sort_modes_t sort_modes;
    config::settings::view_t *settings;
    wstring title;
    wstring dir_name;
    HANDLE panel;
};

using view_ptr_t = std::shared_ptr<view_abstract>;

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467