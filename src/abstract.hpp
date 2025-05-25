#ifndef ABSTRACT_HPP_B6963CFC_532C_4880_B147_F371CF3F33CF
#define ABSTRACT_HPP_B6963CFC_532C_4880_B147_F371CF3F33CF
#pragma once

#include "stdafx.h"
#include "spotify/common.hpp"

namespace spotifar {

struct plugin_interface
{
    virtual ~plugin_interface() {}
    virtual bool is_player_visible() const = 0;
    virtual spotify::api_weak_ptr_t get_api() = 0;
};

using plugin_ptr_t = std::shared_ptr<plugin_interface>;
using plugin_weak_ptr_t = std::weak_ptr<plugin_interface>;

} // namespace spotifar

#endif // ABSTRACT_HPP_B6963CFC_532C_4880_B147_F371CF3F33CF