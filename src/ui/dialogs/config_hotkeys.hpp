#ifndef CONFIG_HOTKEYS_HPP_FC5779E0_D207_4BD1_9E4F_5161BE9135E2
#define CONFIG_HOTKEYS_HPP_FC5779E0_D207_4BD1_9E4F_5161BE9135E2
#pragma once

#include "stdafx.h"
#include "dialog.hpp"

namespace spotifar { namespace ui {

class config_hotkeys_dialog: public dialog
{
public:
    config_hotkeys_dialog();
protected:
    auto handle_result(intptr_t dialog_run_result) -> intptr_t;
    auto handle_key_pressed(int ctrl_id, int combined_key) -> bool;
};

} // namespace ui
} // namespace spotifar

#endif // CONFIG_HOTKEYS_HPP_FC5779E0_D207_4BD1_9E4F_5161BE9135E2