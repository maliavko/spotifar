#ifndef DIALOG_HPP_F17EE340_B0F0_404F_B684_7BBB4C496B88
#define DIALOG_HPP_F17EE340_B0F0_404F_B684_7BBB4C496B88
#pragma once

#include "stdafx.h"

namespace spotifar { namespace ui {
    
FarDialogItem ctrl(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
    FARDIALOGITEMFLAGS flags, const wchar_t *data = L"");

class dialog
{
public:
    //typedef bool (dialog::*event_handler_t)(void*);
    typedef std::function<bool(void*)> event_handler_t;
    typedef std::unordered_map<FARMESSAGE, event_handler_t> event_handlers_t;
    typedef std::unordered_map<int, event_handlers_t> handlers_map_t;
    typedef std::vector<FarDialogItem> layout_t;
public:
    dialog(const GUID *dlg_guid, int width, int height, const layout_t &layout,
        FARDIALOGFLAGS flags = FDLG_NONE);
    virtual ~dialog();

    auto run() -> bool;

    virtual auto init() -> void {};
    virtual auto cleanup() -> void {};
protected:
    virtual auto handle_result(intptr_t dialog_run_result) -> intptr_t { return FALSE; };
    virtual auto handle_key_pressed(int ctrl_id, int combined_key) -> bool { return FALSE; }
private:
    auto handle_dlg_proc_event(intptr_t msg_id, int control_id, void *param) -> bool;
    friend static auto WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2) -> intptr_t;
protected:
    HANDLE hdlg;
private:
    bool are_dlg_events_suppressed = true;
    bool is_modal;
};

} // namespace ui
} // namespace spotifar

#endif // DIALOG_HPP_F17EE340_B0F0_404F_B684_7BBB4C496B88