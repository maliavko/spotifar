#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class root_view: public view
{
public:
    root_view(api_abstract *api);

    auto get_dir_name() const -> const wstring&;
    auto get_items() -> const items_t*;
    auto get_key_bar_info() -> const key_bar_info_t*;
    auto get_info_lines() -> const info_lines_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;
    auto select_item(const data_item_t *data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
private:
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E