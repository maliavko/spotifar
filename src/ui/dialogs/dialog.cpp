#include "dialog.hpp"
#include "config.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

FarDialogItem ctrl(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
    FARDIALOGITEMFLAGS flags, const wchar_t *data)
{
    return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
}

intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
{
    auto d = far3::dialogs::get_dlg_data<dialog>(hdlg);
    if (msg == DN_INITDIALOG)
    {
        d->init();
        return FALSE;
    }
    else if (msg == DN_CLOSE)
    {
        d->cleanup();
        return TRUE;
    }

    if (d && d->handle_dlg_proc_event(msg, (int)param1, param2))
        return TRUE;

    return config::ps_info.DefDlgProc(hdlg, msg, param1, param2);
}

dialog::dialog(const GUID *dlg_guid, int width, int height, const layout_t &layout, FARDIALOGFLAGS flags):
    is_modal((flags & FDLG_NONMODAL) == 0)
{
    hdlg = config::ps_info.DialogInit(&MainGuid, dlg_guid, -1, -1, width, height, 0,
        &layout[0], layout.size(), 0, flags, &dlg_proc, this);

    are_dlg_events_suppressed = false;
}

dialog::~dialog()
{
    if (is_modal)
        config::ps_info.DialogFree(hdlg);
}

bool dialog::run()
{
    if (is_modal)
        return handle_result(config::ps_info.DialogRun(hdlg)) == TRUE;
    return false;
}

bool dialog::handle_dlg_proc_event(intptr_t msg_id, int control_id, void *param)
{
    using utils::keys::make_combined;

    if (are_dlg_events_suppressed)
        return false;
        
    if (msg_id == DN_CONTROLINPUT)
    {
        INPUT_RECORD *ir = reinterpret_cast<INPUT_RECORD*>(param);
        switch (ir->EventType)
        {
            case KEY_EVENT:
                if (ir->Event.KeyEvent.bKeyDown)
                    return handle_key_pressed(control_id, make_combined(ir->Event.KeyEvent));
        }
    }

    return false;
}

} // namespace ui
} // namespace spotifar