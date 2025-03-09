#ifndef ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artist_view: public view
{
public:
    struct album_user_data_t: public user_data_t
    {
        string release_year;
        size_t tracks_total;
        
        static void WINAPI free(void *const user_data,
            const FarPanelItemFreeInfo *const info);
    };
public:
    artist_view(api_abstract *api, const artist &artist);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;
    auto get_sort_modes() const -> const sort_modes_t*;
    auto get_default_settings() const -> config::settings::view_t { return {}; }

    auto select_item(const user_data_t* data) -> intptr_t;
    auto request_extra_info(const user_data_t* data) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto process_key_input(int combined_key) -> intptr_t;
    auto get_free_user_data_callback() -> FARPANELITEMFREECALLBACK;
    auto compare_items(const sort_mode_t &sort_mode, const user_data_t *data1,
        const user_data_t *data2) -> intptr_t;
private:
    api_abstract *api_proxy;
    artist artist;
};

} // namespace ui
} // namespace spotifar

#endif // ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61