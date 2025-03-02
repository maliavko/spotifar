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
    struct find_processor: public view::find_processor
    {
        api_abstract *api_proxy;
        string artist_id;

        find_processor(api_abstract *api, const string &artist_id):
            api_proxy(api), artist_id(artist_id) {};
        auto get_items() const -> const items_t*;
    };
public:
    artist_view(api_abstract *api, const artist &artist);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() const -> const items_t*;
    auto get_find_processor(const string &album_id) -> std::shared_ptr<view::find_processor>;

    auto select_item(const string &album_id) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
private:
    artist artist;
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61