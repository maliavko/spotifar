#ifndef COLLECTION_HPP_59920EC5_429D_4ABC_812A_38611F117C3A
#define COLLECTION_HPP_59920EC5_429D_4ABC_812A_38611F117C3A
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class collection_view: public view
{
public:
    collection_view(api_abstract *api);

    auto get_dir_name() const -> const wstring&;
    auto get_items() -> const items_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;
    auto select_item(const data_item_t *data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto request_extra_info(const data_item_t *data) -> bool;
private:
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // COLLECTION_HPP_59920EC5_429D_4ABC_812A_38611F117C3A