#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class tracks_base_view: public view
{
public:
    struct track_user_data_t: public user_data_t
    {
        wstring track_number;
        int duration_ms;
    };
public:
    tracks_base_view(api_abstract *api, const string &view_uid);

    auto get_sort_modes() const -> const sort_modes_t&;
    auto select_item(const user_data_t* data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
protected:
    auto compare_items(const sort_mode_t &sort_mode, const user_data_t *data1,
        const user_data_t *data2) -> intptr_t;
    auto process_key_input(int combined_key) -> intptr_t;

    virtual auto goto_root_folder() -> bool = 0;
    virtual auto start_playback(const string &track_id) -> bool = 0;
    virtual auto pack_custom_columns(std::vector<wstring> &columns, const simplified_track &t) -> void;
protected:
    api_abstract *api_proxy;
};


/// @brief Showing the list of the given `album` tracks
class album_tracks_view: public tracks_base_view
{
public:
    album_tracks_view(api_abstract *api, const album &album);
    
    auto get_items() -> const items_t*;
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto goto_root_folder() -> bool;
    auto start_playback(const string &track_id) -> bool;
private:
    album album;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB