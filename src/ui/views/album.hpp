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
    struct find_processor: public view::find_processor
    {
        api_abstract *api_proxy;
        string album_id, artist_id;

        find_processor(api_abstract *api, const string &artist_id, const string &album_id):
            api_proxy(api), album_id(album_id), artist_id(artist_id) {};
        auto get_items() const -> const items_t*;
    };
public:
    album_view(api_abstract *api, const artist &artist, const album &album);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_item_idx(const string &item_id) -> size_t;
    auto get_items() const -> const items_t* { return &items; }

    auto select_item(const string &track_id) -> intptr_t;
private:
    album album;
    artist artist;
    api_abstract *api_proxy;

    items_t items;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB