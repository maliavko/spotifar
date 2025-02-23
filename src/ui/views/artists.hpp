#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artists_view: public view
{
public:
    artists_view(api_abstract *api);

    virtual const wchar_t* get_dir_name() const;
    virtual const wchar_t* get_title() const;

    virtual auto get_items() -> const items_t* { return &items; }
    virtual void update_panel_info(OpenPanelInfo *info);

    virtual intptr_t select_item(const string &artist_id);

private:
    api_abstract *api_proxy;
    items_t items;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0