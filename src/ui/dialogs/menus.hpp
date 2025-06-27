#ifndef MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#define MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#pragma once

#include "ui/types.hpp"
#include "config.hpp"
#include "ui/dialogs/dialog.hpp"
#include "ui/dialogs/waiting.hpp"

namespace spotifar { namespace ui {

/// @brief Shows a settings menu with the all config sub-dialogs
intptr_t show_settings_menu();


/// @brief Opens a sort dialog with the all available sort modes
/// for the given `view`. If a new mode is picked, its index will
/// be returned; if the dialog is closed via Esc - "-1"
intptr_t show_sort_dialog(const view_ptr_t v);


/// @brief Shows a splash loading screen in the middle of the Far Manager's panels.
/// If the `message` is not provided, the default one is shown.
/// @note the message is closed automatically by the next panels redrawal
template<typename... Args>
static void show_waiting(int msg_id, Args&&... args)
{
    waiting::show(utils::far3::get_vtext(msg_id, args...));
}

static void hide_waiting()
{
    waiting::hide();
}

} // namespace ui
} // namespace spotifar

#endif //MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C