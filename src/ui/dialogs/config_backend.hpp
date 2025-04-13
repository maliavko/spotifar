#ifndef CONFIG_BACKEND_HPP_831A8F13_0C98_49B9_857E_21CE39CC30A2
#define CONFIG_BACKEND_HPP_831A8F13_0C98_49B9_857E_21CE39CC30A2
#pragma once

#include "stdafx.h"
#include "dialog.hpp"

namespace spotifar { namespace ui {

class config_backend_dialog: public modal_dialog
{
public:
    config_backend_dialog();
protected:
    auto handle_result(intptr_t dialog_run_result) -> intptr_t;
};

} // namespace ui
} // namespace spotifar

#endif // CONFIG_BACKEND_HPP_831A8F13_0C98_49B9_857E_21CE39CC30A2