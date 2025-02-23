#ifndef ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artist_view: public view
{
public:
    artist_view(api_abstract *api, const artist &artist);

    virtual const wchar_t* get_dir_name() const;
    virtual const wchar_t* get_title() const;

    virtual auto get_items() -> const items_t* { return &items; }

    virtual intptr_t select_item(const string &album_id);
    virtual intptr_t process_input(const ProcessPanelInputInfo *info);
private:
    artist artist;
    api_abstract *api_proxy;
    items_t items;
};

} // namespace ui
} // namespace spotifar

#endif // ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61