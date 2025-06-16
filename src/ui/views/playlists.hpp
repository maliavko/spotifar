#ifndef PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#define PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief A base class for all the views, representing list of playlists
class playlists_base_view: public view
{
public:
    playlists_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name = L""):
        view(panel, title, dir_name), api_proxy(api)
        {}

    auto get_items() -> const items_t& override;
protected:
    virtual auto get_playlists() -> std::generator<const simplified_playlist_t&> = 0;

    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto select_item(const data_item_t* data) -> intptr_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
    auto process_key_input(int combined_key) -> intptr_t override;
    bool request_extra_info(const data_item_t *data) override;
    auto get_panel_modes() const -> const panel_modes_t* override;
protected:
    api_weak_ptr_t api_proxy;
};


/// @brief A class-view, representing a list of user's saved playlists
class saved_playlists_view: public playlists_base_view
{
public:
    saved_playlists_view(HANDLE panel, api_weak_ptr_t api);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // playlists_base_view
    auto get_playlists() -> std::generator<const simplified_playlist_t&> override;
private:
    saved_playlists_ptr collection;
};


/// @brief A list of recently listened playlists
class recent_playlists_view:
    public playlists_base_view,
    public play_history_observer // on_history_changed
{
public:
    struct history_playlist_t: public playlist_t
    {
        string played_at;
    };
public:
    recent_playlists_view(HANDLE panel, api_weak_ptr_t api);
    ~recent_playlists_view();
protected:
    void rebuild_items();

    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    auto get_sort_modes() const -> const view::sort_modes_t& override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // playlists_base_view interface
    auto get_playlists() -> std::generator<const simplified_playlist_t&>;
    
    // play_history_observer handler
    void on_history_changed() override;
private:
    std::vector<history_playlist_t> items;
};

} // namespace ui
} // namespace spotifar

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F