#include "dialog.hpp"
#include "config.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

FarDialogItem ctrl(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
    FARDIALOGITEMFLAGS flags, const wchar_t *data, const wchar_t *history)
{
    return FarDialogItem(type, x1, y1, x2, y2, {}, history, nullptr, flags, data);
}


static intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
{
    auto d = far3::dialogs::get_dlg_data<modal_dialog>(hdlg);
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

modal_dialog::modal_dialog(const GUID *dlg_guid, int width, int height, const layout_t &layout,
                           const wchar_t *help_topic, FARDIALOGFLAGS flags)
{
    hdlg = config::ps_info.DialogInit(&MainGuid, dlg_guid, -1, -1, width, height, help_topic,
        &layout[0], layout.size(), 0, flags & ~FDLG_NONMODAL, &dlg_proc, this);
}

modal_dialog::~modal_dialog()
{
    config::ps_info.DialogFree(hdlg);
}

bool modal_dialog::run()
{
    return handle_result(config::ps_info.DialogRun(hdlg)) == TRUE;
}

void modal_dialog::close()
{
    utils::far3::dialogs::close(hdlg);
}

bool modal_dialog::handle_dlg_proc_event(intptr_t msg_id, int control_id, void *param)
{
    using utils::keys::make_combined;
        
    if (msg_id == DN_CONTROLINPUT)
    {
        const auto *ir = reinterpret_cast<INPUT_RECORD*>(param);
        switch (ir->EventType)
        {
            case KEY_EVENT:
                if (ir->Event.KeyEvent.bKeyDown)
                    return handle_key_pressed(control_id, make_combined(ir->Event.KeyEvent));
        }
    }
    else if (msg_id == DN_BTNCLICK)
    {
        return handle_btn_clicked(control_id);
    }

    return false;
}

} // namespace ui
} // namespace spotifar