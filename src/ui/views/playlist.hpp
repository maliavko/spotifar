#ifndef PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1
#define PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class playlist_view: public view
{
public:
    struct playlist_track_user_data_t: public user_data_t
    {
        string added_at;
        int duration_ms;
        wstring album_name;
        string album_release_year;
        
        static void WINAPI free(void *const user_data,
            const FarPanelItemFreeInfo *const info);
    };
public:
    playlist_view(api_abstract *api, const playlist &p);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t { return {}; }

    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto select_item(const user_data_t* data) -> intptr_t;
    auto get_free_user_data_callback() -> FARPANELITEMFREECALLBACK;
    auto compare_items(const sort_mode_t &sort_mode,
        const user_data_t *data1, const user_data_t *data2) -> intptr_t;
private:
    api_abstract *api_proxy;
    playlist playlist;
};

} // naemspace ui
} // namespace spotifar

#endif // PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1