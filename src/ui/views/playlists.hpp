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
    playlists_view(api_abstract *api);

    virtual const wchar_t* get_dir_name() const;
    virtual const wchar_t* get_title() const;

    virtual auto get_items() -> const items_t* { return &items; }

    virtual intptr_t select_item(const string &track_id);
private:
    api_abstract *api_proxy;
    items_t items;
};

} // namespace ui
} // namespace spotifar

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F