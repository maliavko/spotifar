#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar { namespace ui {

class album_view: public view
{
public:
    album_view(spotify::api *api, const spotify::artist &artist, const spotify::album &album,
                const spotify::track &initial_track = spotify::track());

    virtual view_items_t get_items();
    virtual intptr_t select_item(const string &track_id);
private:
    spotify::album album;
    spotify::artist artist;
    spotify::track initial_track;
    spotify::api *api;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB