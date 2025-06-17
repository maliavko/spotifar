#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "utils.hpp"
#include "config.hpp"
#include "ui/types.hpp"
#include "spotify/interfaces.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief An abstract class for holding a currently viewable panel's data and
/// business logic. By design, a user can travers through diffrent kind of 
/// Spotify collections, each of them has a different key-features inside.
/// To separate all of them from each other at any moment the panel creates and holds
/// one view and change it by request of the user
class view
{
public:
    /// @param panel a real param handle, PANEL_PASSIVE dose not work
    /// @param title a label used to be shown on top of the panel
    /// @param dir_name a directory name used by Far to uniquely identify the view
    /// and match it respectively to the items on the panels, e.g. while returning back
    /// from the view
    view(HANDLE panel, const wstring &title, const wstring &dir_name = L"");
    view(const view&) = delete;
    virtual ~view() {}

    view& operator=(const view&) = delete;

    // a public interface, used by ui::panel class

    /// @brief a helper event from outside, its called right after the items
    /// are populated on the panel
    void on_items_updated();
    auto compare_items(const CompareInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto select_item(const SetDirectoryInfo *info) -> intptr_t;
    auto get_title() const -> const wstring& { return title; }
    auto get_dir_name() const -> const wstring& { return dir_name; }
    void update_panel_info(OpenPanelInfo *info);
    auto request_extra_info(const PluginPanelItem *item) -> intptr_t;
    
    /// @brief Returns a unique id for the object created
    auto get_uid() const -> uint32_t { return id; }

    /// @brief Searches for the `item_id` item on the panel and
    /// return its index or 0
    auto get_item_idx(const item_id_t &item_id) -> size_t;

    /// @brief Return the view appropriate settings object, the object
    /// is persistently saved to/load from Far config
    auto get_settings() const -> config::settings::view_t*;

    /// @brief Switch sort mode on the current panel
    void select_sort_mode(int sort_mode_idx);

    /// @param callback a handler, which is being called when ".." is hit on the panel
    auto set_return_callback(return_callback_t callback) { return_callback = callback; }
    
    /// @return A handler which is being called when ".." is hit on the panel
    auto get_return_callback() const -> const return_callback_t& { return return_callback; }

    /// @brief Method should return the list of available sort modes, which
    /// will be shown in the separate dialog on the panel 
    virtual auto get_sort_modes() const -> const sort_modes_t& = 0;

    /// @brief Method should return a reference to the list of items,
    /// which panel will represent
    virtual auto get_items() -> const items_t& = 0;

    /// @brief Method should return the list of keys with their modifiers and labels,
    /// which will be shown on the panel's key bar
    virtual auto get_key_bar_info() -> const key_bar_info_t* { return nullptr; }

    /// @brief Method should return a list of info lines, visible for user
    /// via Ctrl+L hotkey tmp panel
    virtual auto get_info_lines() -> const info_lines_t* { return nullptr; }

    /// @brief Method should return the list of panel modes, which can be
    /// user for the particular view
    virtual auto get_panel_modes() const -> const panel_modes_t* { return nullptr; }

    /// @brief A handler used for showing a filtering dialog, by default show nothing.
    /// Called when user hits Ctrl+I combination on the panels
    virtual void show_filters_dialog() {}
protected:
    /// @brief A helper function to unpack user data from the far items
    static auto unpack_user_data(const UserDataItem &user_data) -> const data_item_t*;

    /// @brief Returns a panel handle, the view is associsted with
    auto get_panel_handle() -> HANDLE const { return panel; }
    
    /// @brief Returns ids of the selected items on the panel, if none are
    /// selected, it returns the one item under cursor
    auto get_selected_items() -> item_ids_t;

    /// @brief Returns a unique view string id, used in caching
    string get_type_uid() const { return typeid(*this).name(); }
    
    /// @brief If no saved settings is available for the view, the default ones
    /// return by this method are gonna be used
    virtual auto get_default_settings() const -> config::settings::view_t = 0;

    /// @brief Called when user hits F3 key on the panel, perform additional
    /// calculations for the item under cursor with the given user `data`,
    /// return `true` if panel should be updated
    virtual bool request_extra_info(const data_item_t *data) { return false; }

    /// @brief Called when the folder item is selected on the panel,
    /// usually the view redirects user to another view; return `TRUE` if
    /// the panel should be redrawn
    virtual auto select_item(const data_item_t *data) -> intptr_t { return FALSE; }

    /// @brief Called when the user input is received
    /// @param combined_key is a summed int of a VK_*** key code and
    /// all utils::keys::mods::*** modifiers
    virtual auto process_key_input(int combined_key) -> intptr_t { return FALSE; }

    /// @brief Called when two items `data1` and `data2` on the panel
    /// should be sorted
    /// @param mode current sorting mode selected on th epanel
    virtual auto compare_items(const sort_mode_t &modes, const data_item_t *data1, const data_item_t *data2) -> intptr_t { return -2; }
private:
    bool is_first_init = true; // data-is-set flag
    return_callback_t return_callback;
    sort_modes_t sort_modes;
    config::settings::view_t *settings;
    wstring title; // used for representing folder name as a panel title
    wstring dir_name; // used for associate a view with the item on the panel
    HANDLE panel; // a panel object the view associated with
    uint32_t id; // unique object id
};

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467