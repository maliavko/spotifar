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
    playlist_view(api_abstract *api, const playlist &p);

    auto get_dir_name() const -> const wstring&;
    auto get_items() -> const items_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;
    auto select_item(const spotify::data_item* data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto compare_items(const sort_mode_t &sort_mode,
        const spotify::data_item *data1, const spotify::data_item *data2) -> intptr_t;
private:
    api_abstract *api_proxy;
    playlist playlist;
};

} // naemspace ui
} // namespace spotifar

#endif // PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1