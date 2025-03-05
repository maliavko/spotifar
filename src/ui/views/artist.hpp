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
    artist_view(api_abstract *api, const artist &artist);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;

    auto select_item(const SetDirectoryInfo *info) -> intptr_t;
    auto request_extra_info(const string &artist_id) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
private:
    api_abstract *api_proxy;
    artist artist;
};

} // namespace ui
} // namespace spotifar

#endif // ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61