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
    playlist_view(api_abstract *api, const playlist &p);

    virtual const wchar_t* get_dir_name() const;
    virtual const wchar_t* get_title() const;

    virtual auto get_items() -> const items_t* { return &items; }

    virtual intptr_t select_item(const string &track_id);
private:
    api_abstract *api_proxy;
    playlist playlist;
    items_t items;
};

} // naemspace ui
} // namespace spotifar

#endif // PLAYLIST_HPP_62C1BAAD_7EF8_4237_9EB8_5E6A809630B1