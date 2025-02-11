#ifndef PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1
#define PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class playlist_view: public view
{
public:
    playlist_view(api *api, const playlist &p);

    virtual view_items_t get_items();
    virtual std::shared_ptr<view> select_item(const string &track_id);

    static std::shared_ptr<playlist_view> build(api *api, const playlist &p);
private:
    playlist playlist;
    api *api;
};

} // naemspace ui
} // namespace spotifar

#endif // PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1