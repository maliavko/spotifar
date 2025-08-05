#include "ui/dialogs/waiting.hpp"
#include "ui/dialogs/dialog.hpp"
#include "config.hpp"
#include "spotifar.hpp"
#include "plugin.h" // IWYU pragma: keep
#include "spotify/api.hpp" // IWYU pragma: keep
#include "utils.hpp"
#include <windows.h>

namespace spotifar { namespace ui {

// last set waiting message
static wstring message = L"";

// amount of ticks accunulated, if the number exists some max value,
// the value is reset to 0
static size_t ticker = 0;

// waiting dialog handler if it exists at the moment
static HANDLE hdlg = NULL;

enum controls : int
{
    dialog_box,
    message_lbl,
};

static const int
    width = 50, height = 7,
    center_x = width / 2,
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,    DIF_NONE),
    ctrl(DI_TEXT,        view_x1, view_y1+1, view_x2, 1,    DIF_CENTERTEXT)
};

static intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t control_id, void *param)
{
    if (msg == DN_CONTROLINPUT)
    {
        // blocking possibility to close waiting by escape e.g.
        return TRUE;
    }

    return config::ps_info.DefDlgProc(hdlg, msg, control_id, param);
}

/// @note if `text` is empty, that means we keep the message, but updating ticker
/// to represent some visible progress
static void update_message(const wstring &text = L"")
{
    static wchar_t msg[64];

    // as the waiting is called from different threads, I hope it
    // will help me to avoid crashes
    static std::mutex guard;

    std::lock_guard lock(guard);

    if (!text.empty())
        message = text;
    
    auto s = std::format(L"{}{:<3}", message, wstring(ticker, L'.'));
    config::fsf.snprintf(msg, std::size(msg), s.c_str());
    utils::far3::dialogs::set_text(hdlg, message_lbl, msg);
}

void waiting::show(const wstring &msg)
{
    if (hdlg == NULL)
    {
        hdlg = config::ps_info.DialogInit(&MainGuid, &SplashDialogGuid, -1, -1, width, height, L"",
            &dlg_items_layout[0], dlg_items_layout.size(), 0, FDLG_NONMODAL, &dlg_proc, NULL);
    }

    update_message(msg);
    ticker = 0;
}

void waiting::hide()
{
    if (hdlg != NULL)
    {
        // delaying waiting dialog closure, to make the process visually smoother
        utils::far3::synchro_tasks::push(
            [] {
                utils::far3::dialogs::close(hdlg);
                hdlg = NULL;
            });
    }
}

void waiting::tick(const utils::clock_t::duration &delta)
{
    static const size_t max_ticks_count = 4;
    static utils::clock_t::duration accumulated{}, period = 350ms;

    if (hdlg == NULL) return;

    accumulated += delta;
    if (accumulated > period)
    {
        utils::far3::synchro_tasks::push([] { update_message(); });
        
        if (++ticker >= max_ticks_count)
            ticker = 0;

        accumulated = accumulated % period;
    }

    // collections fetching requests can suspend the threads, while waiting for the
    // endpoints availability; hitting Escape button during this time period, when
    // the user sees the waiting splash dialog, allows to break this freeze, however
    // the main thread can also be suspended, so the button pressed condition is checked
    // separately in the forground thread
    if (utils::keys::is_pressed(VK_ESCAPE))
    {
        if (auto plugin = get_plugin())
            if (auto api = plugin->get_api())
                api->cancel_pending_requests();
    }
}

} // namespace ui
} // namespace spotifar