#ifndef ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

class artist_view: public view
{
public:
    artist_view(spotify::api_abstract *api, const spotify::artist &artist);

    virtual view_items_t get_items();
    virtual intptr_t select_item(const string &album_id);
    virtual intptr_t process_input(const ProcessPanelInputInfo *info);
private:
    spotify::artist artist;
    spotify::api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61