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
    artists_view(api_abstract *api);

    auto get_dir_name() const -> const wstring&;
    auto get_items() -> const items_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;
    auto select_item(const data_item_t *data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto request_extra_info(const data_item_t *data) -> bool;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;
private:
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0