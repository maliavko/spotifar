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
    // @brief Returns total amount of user's playlists
    struct find_processor: public view::find_processor
    {
        api_abstract *api_proxy;

        find_processor(api_abstract *api): api_proxy(api) {};
        auto get_items() const -> const items_t*;
    };

public:
    playlists_view(api_abstract *api);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() const -> const items_t* { return &items; }
    auto get_find_processor(const string &item_id) -> std::shared_ptr<view::find_processor>;

    auto select_item(const string &track_id) -> intptr_t;
private:
    api_abstract *api_proxy;
    items_t items;
};

} // namespace ui
} // namespace spotifar

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F