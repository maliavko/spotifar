#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artists_view: public view
{
public:
    artists_view(spotify::api *api);

    virtual view_items_t get_items();
    virtual std::shared_ptr<view> select_item(const string &artist_id);
    
    static std::shared_ptr<artists_view> build(spotify::api *api);

private:
    spotify::api *api;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0