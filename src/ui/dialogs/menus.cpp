#include "config.hpp"
#include "lng.hpp"
#include "utils.hpp"
#include "menus.hpp"
#include "ui/events.hpp"
#include "ui/views/view.hpp"
#include "settings/hotkeys.hpp"
#include "settings/general.hpp"
#include "settings/backend.hpp"

namespace spotifar { namespace ui {

using namespace utils;
using namespace utils::far3;

using far3::get_text;

intptr_t show_settings_menu()
{
    static FarMenuItem items[] = {
        { MIF_NONE, get_text(MCfgDlgMenuGeneral) },
        { MIF_NONE, get_text(MCfgDlgMenuBackend) },
        { MIF_NONE, get_text(MCfgDlgMenuHotkeys) },
    };

    while (1)
    {
        auto opt_idx = config::ps_info.Menu(
            &MainGuid,
            &ConfigMenuGuid,
            -1, -1, 0, // centered, autowidth
            FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE,
            get_text(MCfgDlgMenuTitle), L"", L"Config", // title, bottom, helptopic
            {}, {}, // no break codes
            items, std::size(items)
        );

        if (opt_idx == -1)
            return TRUE;
            
        // select a last picked option, so when the dialog is redrawn, it is show correctly
        for(auto &i: items) i.Flags = MIF_NONE;
        items[opt_idx].Flags |= MIF_SELECTED;

        if (opt_idx == 0)
        {
            settings::general_dialog().run();
        }
        else if (opt_idx == 1)
        {
            settings::backend_dialog().run();
        }
        else if (opt_idx == 2)
        {
            settings::hotkeys_dialog().run();
        }
    }
    return TRUE;
}

intptr_t show_sort_dialog(const view_ptr_t v)
{
    const auto &modes = v->get_sort_modes();
    const auto &settings = v->get_settings();

    std::vector<FarMenuItem> result;
    for (size_t idx = 0; idx < modes.size(); idx++)
    {
        const auto &sm = modes[idx];

        // the width of the dialog is 28, the sort mode name is aligned to the left,
        // while the hotkey name - to the right
        wstring label = std::format(L"{: <18}{: >10}", sm.name,
            keys::combined_to_string(sm.get_combined_key()));

        MENUITEMFLAGS flags = MIF_NONE;
        if (idx == settings->sort_mode_idx)
            flags |= MIF_CHECKED | (settings->is_descending ? L'▼' : L'▲');

        result.push_back({ flags, _wcsdup(label.c_str()) });
    }

    auto info = far3::panels::get_info(PANEL_ACTIVE);

    auto sort_idx = config::ps_info.Menu(
        &MainGuid,
        &SortDialogGuid,
        info.PanelRect.left + 5, -1, 0, // horizontally alligned to the active panel left
        FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE,
        get_text(MSortDlgTitle), L"", L"SortDialog", // title, bottom, helptopic
        {}, {}, // no break codes
        &result[0], result.size()
    );

    for (auto &item: result)
        free(const_cast<wchar_t*>(item.Text));

    return sort_idx;
}

scoped_waiting::scoped_waiting(int msg_id)
{
    ss = config::ps_info.SaveScreen(0, 0, -1, -1);
    show_waiting(msg_id);
}

scoped_waiting::~scoped_waiting()
{
    config::ps_info.RestoreScreen(ss);
}

} // namespace ui
} // namespace spotifar