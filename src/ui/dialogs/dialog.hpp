#ifndef DIALOG_HPP_F17EE340_B0F0_404F_B684_7BBB4C496B88
#define DIALOG_HPP_F17EE340_B0F0_404F_B684_7BBB4C496B88
#pragma once

#include "stdafx.h"

namespace spotifar { namespace ui {
    
FarDialogItem ctrl(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
    FARDIALOGITEMFLAGS flags, const wchar_t *data = L"");

class modal_dialog
{
public:
    using layout_t = std::vector<FarDialogItem>;
public:
    modal_dialog(const GUID *dlg_guid, int width, int height, const layout_t &layout,
        FARDIALOGFLAGS flags = FDLG_NONE);
    virtual ~modal_dialog();

    bool run();

    virtual void init() {};
    virtual void cleanup() {};
protected:
    virtual auto handle_result(intptr_t dialog_run_result) -> intptr_t { return FALSE; };
    virtual bool handle_key_pressed(int ctrl_id, int combined_key) { return FALSE; }
private:
    bool handle_dlg_proc_event(intptr_t msg_id, int control_id, void *param);
    friend static auto WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2) -> intptr_t;
protected:
    HANDLE hdlg;
};

} // namespace ui
} // namespace spotifar

#endif // DIALOG_HPP_F17EE340_B0F0_404F_B684_7BBB4C496B88