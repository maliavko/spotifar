#ifndef DIALOGS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#define DIALOGS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#pragma once

#include "stdafx.h"
#include "views/view.hpp"

namespace spotifar { namespace ui {

struct config_dialog
{
    static bool show();
};

struct sort_dialog
{
    static int show(const view &v);
};

} // namespace ui
} // namespace spotifar

#endif //DIALOGS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C