#ifndef PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1
#define PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

// TODO: move to tracks and derive from tracks_base_view?
class playlist_view: public view_abstract
{
public:
    playlist_view(api_proxy_ptr api, const playlist_t &p);
    ~playlist_view() { api_proxy.reset(); }

    auto get_items() -> const items_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;
    auto select_item(const data_item_t* data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto compare_items(const sort_mode_t &sort_mode,
        const data_item_t *data1, const data_item_t *data2) -> intptr_t;
private:
    api_proxy_ptr api_proxy;
    playlist_t playlist;
    saved_tracks_ptr collection;
};

} // naemspace ui
} // namespace spotifar

#endif // PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1