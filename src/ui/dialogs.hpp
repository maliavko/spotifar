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
    /// @brief Opens a sort dialog with the all available sort modes
    /// for the given `view`. If a new mode is picked, its index will
    /// be returned; if the dialog is closed via Esc - "-1"
    static int show(const view &v);
};

} // namespace ui
} // namespace spotifar

#endif //DIALOGS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C