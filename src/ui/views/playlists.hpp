#ifndef PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#define PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"
#include "spotify/playback.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class playlists_base_view: public view
{
public:
    playlists_base_view(api_abstract *api, const string &view_uid,
        return_callback_t callback);

    auto get_items() -> const items_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto select_item(const data_item_t* data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;
    auto process_key_input(int combined_key) -> intptr_t;
    auto request_extra_info(const data_item_t* data) -> bool;

    virtual auto get_playlists() -> std::generator<const simplified_playlist_t&> = 0;
protected:
    api_abstract *api_proxy;
};

class playlists_view: public playlists_base_view
{
public:
    playlists_view(api_abstract *api);

    auto get_dir_name() const -> const wstring&;
protected:
    auto get_default_settings() const -> config::settings::view_t;
    auto get_playlists() -> std::generator<const simplified_playlist_t&>;
private:
    api_abstract *api_proxy;
    saved_playlists_ptr collection;
};


/// @brief 
class recent_playlists_view:
    public playlists_base_view,
    public play_history_observer
{
public:
    struct history_playlist_t: public playlist_t
    {
        string played_at;

        history_playlist_t(const string &played_at, const playlist_t &playlist):
            playlist_t(playlist), played_at(played_at)
            {}
    };
public:
    recent_playlists_view(api_abstract *api);
    ~recent_playlists_view();
    
    auto get_dir_name() const -> const wstring&;
protected:
    auto rebuild_items() -> void;

    auto get_default_settings() const -> config::settings::view_t;
    auto get_sort_modes() const -> const view::sort_modes_t&;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;

    auto get_playlists() -> std::generator<const simplified_playlist_t&>;
    
    void on_items_changed();
private:
    std::vector<history_playlist_t> items;
};

} // namespace ui
} // namespace spotifar

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F