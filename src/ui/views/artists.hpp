#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artists_view: public view
{
public:
    struct artist_user_data_t: public user_data_t
    {
        size_t popularity;
        size_t followers_count;
        
        static void WINAPI free(void *const user_data,
            const FarPanelItemFreeInfo *const info);
    };
public:
    artists_view(api_abstract *api);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;
    auto get_sort_modes() -> const sort_modes_t*;

    auto select_item(const user_data_t *data) -> intptr_t;
    auto request_extra_info(const user_data_t *data) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto get_free_user_data_callback() -> FARPANELITEMFREECALLBACK;
    auto compare_items(view::sort_mode_t sort_mode, const user_data_t *data1,
        const user_data_t *data2) -> intptr_t;
private:
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0