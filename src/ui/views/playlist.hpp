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
    struct find_processor: public view::find_processor
    {
        api_abstract *api_proxy;
        const string playlist_id;

        find_processor(api_abstract *api, const string &playlist_id):
            api_proxy(api), playlist_id(playlist_id) {};
        auto get_items() const -> const items_t*;
    };
public:
    playlist_view(api_abstract *api, const playlist &p);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() const -> const items_t* { return &items; }

    auto select_item(const string &track_id) -> intptr_t;
private:
    api_abstract *api_proxy;
    playlist playlist;
    items_t items;
};

} // naemspace ui
} // namespace spotifar

#endif // PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1