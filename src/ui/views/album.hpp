#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

class album_view: public view
{
public:
    album_view(spotify::api_abstract *api, const spotify::artist &artist, const spotify::album &album);

    virtual const wchar_t* get_dir_name() const;
    virtual const wchar_t* get_title() const;

    virtual items_t get_items() { return items; }
    virtual intptr_t select_item(const string &track_id);
    virtual size_t get_item_idx(const string &item_id);
protected:
    void rebuild_items();
private:
    spotify::album album;
    spotify::artist artist;
    spotify::api_abstract *api_proxy;

    items_t items;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB