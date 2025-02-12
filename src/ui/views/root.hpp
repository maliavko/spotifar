#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar { namespace ui {

class root_view: public view
{
public:
    root_view(spotify::api *api);

    virtual view_items_t get_items();
    virtual intptr_t select_item(const string &view_id);
private:
    spotify::api *api;
};

} // namespace ui
} // namespace spotifar

#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E