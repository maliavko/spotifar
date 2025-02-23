#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

class root_view: public view
{
public:
    root_view(spotify::api_abstract *api);

    virtual const wchar_t* get_dir_name() const;
    virtual const wchar_t* get_title() const;

    virtual items_t get_items();
    virtual intptr_t select_item(const string &view_id);
    virtual auto get_key_bar_info() -> const key_bar_info_t*;
    virtual auto get_info_lines() -> const info_lines_t*;
private:
    spotify::api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E