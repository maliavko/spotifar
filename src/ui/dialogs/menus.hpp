#ifndef MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#define MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#pragma once

#include "stdafx.h"
#include "../views/view.hpp"

namespace spotifar { namespace ui {

intptr_t show_settings_menu();

/// @brief Opens a sort dialog with the all available sort modes
/// for the given `view`. If a new mode is picked, its index will
/// be returned; if the dialog is closed via Esc - "-1"
intptr_t show_sort_dialog(const view_abstract &v);

/// @brief Shows a splash loading screen in the middle of the Far Manager's panels.
/// If the `message` is not provided, the default one is shown.
/// @note the message is closed automatically by the next panels redrawal
template<typename... Args>
void show_waiting(int msg_id, Args&&... args)
{
    static const wchar_t* msgs[] = { L"", L"" };
    static wstring msg;
    
    msg = utils::far3::get_vtext(msg_id, args...);
    msgs[1] = msg.c_str();
    
    config::ps_info.Message(&MainGuid, &SplashDialogGuid, 0, L"", msgs, std::size(msgs), 0);
}

} // namespace ui
} // namespace spotifar

#endif //MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C