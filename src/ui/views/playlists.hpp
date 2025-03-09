#ifndef PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#define PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class playlists_view: public view
{
public:
    playlists_view(api_abstract *api);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;
    auto get_default_settings() const -> config::settings::view_t { return {}; }

    auto select_item(const user_data_t* data) -> intptr_t;
    auto request_extra_info(const user_data_t* data) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
private:
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F