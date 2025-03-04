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
    album_view(api_abstract *api, const artist &artist, const album &album);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;

    auto select_item(const string &track_id) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
private:
    album album;
    artist artist;
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB