#ifndef ABSTRACT_HPP_B6963CFC_532C_4880_B147_F371CF3F33CF
#define ABSTRACT_HPP_B6963CFC_532C_4880_B147_F371CF3F33CF
#pragma once

#include "stdafx.h"
#include "ui/player.hpp"

namespace spotifar {

struct plugin_interface
{
    virtual ~plugin_interface() {}
    
    virtual const std::unique_ptr<ui::player>& get_player() const = 0;
};

} // namespace spotifar

#endif // ABSTRACT_HPP_B6963CFC_532C_4880_B147_F371CF3F33CF