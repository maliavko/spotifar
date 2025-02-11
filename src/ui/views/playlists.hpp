#ifndef PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#define PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class playlists_view: public view
{
public:
    playlists_view(api *api);

    virtual view_items_t get_items();
    virtual std::shared_ptr<view> select_item(const string &track_id);

    static std::shared_ptr<playlists_view> build(api *api);
private:
    api *api;
};

} // namespace ui
} // namespace spotifar

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F