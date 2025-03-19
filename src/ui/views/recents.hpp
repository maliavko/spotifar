#ifndef RECENTS_HPP_4C5B011D_468F_4841_B5E3_0B430BA781EF
#define RECENTS_HPP_4C5B011D_468F_4841_B5E3_0B430BA781EF
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class recents_view: public view
{
public:
    recents_view(api_abstract *api);

    auto get_dir_name() const -> const wstring&;
    auto get_items() -> const items_t*;
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

#endif // RECENTS_HPP_4C5B011D_468F_4841_B5E3_0B430BA781EF