#ifndef BACKEND_HPP_831A8F13_0C98_49B9_857E_21CE39CC30A2
#define BACKEND_HPP_831A8F13_0C98_49B9_857E_21CE39CC30A2
#pragma once

#include "stdafx.h"
#include "../dialog.hpp"

namespace spotifar { namespace ui { namespace settings {

class backend_dialog: public modal_dialog
{
public:
    backend_dialog();
protected:
    void init() override;
    auto handle_result(intptr_t dialog_run_result) -> intptr_t override;
};

} // namespace settings
} // namespace ui
} // namespace spotifar

#endif // BACKEND_HPP_831A8F13_0C98_49B9_857E_21CE39CC30A2