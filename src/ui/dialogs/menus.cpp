#include "config.hpp"
#include "utils.hpp"
#include "menus.hpp"
#include "config_hotkeys.hpp"
#include "config_general.hpp"

namespace spotifar { namespace ui {

using namespace utils;
using namespace utils::far3;

intptr_t show_config_menu()
{
    static FarMenuItem items[] = {
        { MIF_NONE, far3::get_text(MConfigDialogMenuGeneralOpt) },
        { MIF_NONE, far3::get_text(MConfigDialogMenuBackendOpt) },
        { MIF_NONE, far3::get_text(MConfigDialogMenuHotkeysOpt) },
    };

    while (1)
    {
        auto opt_idx = config::ps_info.Menu(
            &MainGuid,
            &ConfigMenuGuid,
            -1, -1, 0, // centered, autowidth
            FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE,
            far3::get_text(MConfigDialogMenuTitle), L"",
            {}, {}, {}, // no break codes, no help topic
            items, std::size(items)
        );

        if (opt_idx == -1)
            return TRUE;
            
        // select a last picked option, so when the dialog is redrawn, it is show correctly
        for(auto &i: items) i.Flags = MIF_NONE;
        items[opt_idx].Flags |= MIF_SELECTED;

        if (opt_idx == 0)
        {
            config_general_dialog().run();
        }
        else if (opt_idx == 2)
        {
            config_hotkeys_dialog().run();
        }
    }
    return TRUE;
}

intptr_t show_sort_dialog(const view_abstract &v)
{
    const auto &modes = v.get_sort_modes();
    const auto &settings = v.get_settings();

    std::vector<FarMenuItem> result;
    for (size_t idx = 0; idx < modes.size(); idx++)
    {
        auto &sm = modes[idx];

        // the width of the dialog is 28, the sort mode name is aligned to the left,
        // while the hotkey name - to the right
        wstring label = std::format(L"{: <18}{: >10}", sm.name,
            keys::combined_to_string(sm.combined_key));

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
        far3::get_text(MSortDialogTitle), L"",
        {}, {}, {}, // no break codes, no help topic
        &result[0], result.size()
    );

    for (auto &item: result)
        free(const_cast<wchar_t*>(item.Text));

    return sort_idx;
}

namespace collections_filter
{
    intptr_t show()
    {
        PluginDialogBuilder builder(config::ps_info, MainGuid, ConfigSpotifyDialogGuid, L"Test Dialog", NULL);
    
        int radio_idx = -1;
        const int btns[] {
            MPanelArtistsItemLabel, MPanelTracksItemLabel, MPanelAlbumsItemLabel, MPanelPlaylistsItemLabel
        };
        builder.AddRadioButtons(&radio_idx, 4, btns);
    
        builder.AddSeparator();
    
        static int selected_item;
        static const wchar_t* items[] = {
            L"11111111111 | 11111 | 1111",
            L"22222222222 | 22222 | 2222",
            L"33333333333 | 33333 | 3333",
        };
        builder.AddListBox(&selected_item, 40, 10, items, 3, DIF_LISTNOBOX);
    
        builder.AddOKCancel(MOk, MCancel);
    
        auto result = builder.ShowDialogEx();
        if (result >= 0)
            return radio_idx;
        return result;
    }
}

} // namespace ui
} // namespace spotifar