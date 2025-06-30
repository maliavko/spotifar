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
    // waiting does not work if there are modal windows visible, we close them
    // in such cases
    auto wcount = utils::far3::actl::get_windows_count();
    for (size_t idx = 0; idx < wcount; idx++)
        if (auto winfo = utils::far3::actl::get_window_info(idx); winfo && winfo->Flags & WIF_MODAL)
            utils::far3::dialogs::close((HANDLE)winfo->Id);

    waiting::show(utils::far3::get_vtext(msg_id, args...));
}

static void hide_waiting()
{
    waiting::hide();
}

/// @brief Shows a splash loading screen with the formatted message of
/// PsInfo.Text(msg_id) and given `args`. The message differes from the regular
/// waiting dialog, being a simple PsInfo.Message, which will get closed
/// automatically after the first screen refresh.
/// @note ideal for the cases when an operation is blocking
template<typename... Args>
void show_simple_waiting(int msg_id, Args&&... args)
{
    static wstring message;
    message = std::format(L"{: ^50}", utils::far3::get_vtext(msg_id, args...));
    
    static const wchar_t* msgs[] = { L"", L"", L"", L"" };
    msgs[2] = message.c_str();

    config::ps_info.Message(&MainGuid, &SplashDialogGuid, 0, L"", msgs, std::size(msgs), 0);
}

struct scoped_waiting
{
    scoped_waiting(int msg_id, auto&&... args)
    {
        show_waiting(msg_id, std::forward<decltype(args)>(args)...);
    }

    ~scoped_waiting()
    {
        hide_waiting();
    }
};

} // namespace ui
} // namespace spotifar

#endif //MENUS_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C