#ifndef CONFIG_GENERAL_HPP_FC5779E0_D207_4BD1_9E4F_5161BE9135E2
#define CONFIG_GENERAL_HPP_FC5779E0_D207_4BD1_9E4F_5161BE9135E2
#pragma once

#include "stdafx.h"
#include "dialog.hpp"

namespace spotifar { namespace ui {

class config_general_dialog: public modal_dialog
{
public:
    config_general_dialog();
protected:
    void init() override;
    auto handle_result(intptr_t dialog_run_result) -> intptr_t override;
};

} // namespace ui
} // namespace spotifar

#endif // CONFIG_GENERAL_HPP_FC5779E0_D207_4BD1_9E4F_5161BE9135E2