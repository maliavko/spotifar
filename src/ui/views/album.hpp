#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class album_view: public view
{
public:
    struct track_user_data_t: public user_data_t
    {
        wstring track_number;
        int duration_ms;
    };
public:
    album_view(api_abstract *api, const album &album);

    auto get_dir_name() const -> const wstring&;
    auto get_items() -> const items_t*;
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;

    auto select_item(const user_data_t* data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
protected:
    auto compare_items(const sort_mode_t &sort_mode, const user_data_t *data1,
        const user_data_t *data2) -> intptr_t;
private:
    album album;
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB