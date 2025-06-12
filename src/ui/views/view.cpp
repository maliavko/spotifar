#include "view.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

namespace panels = utils::far3::panels;

view::panel_mode_t::panel_mode_t(std::vector<panel_mode_t::column_t> &&cols, bool is_wide): is_wide(is_wide)
{
    columns = std::move(cols);
    rebuild();
}

void view::panel_mode_t::insert_column(const column_t &col, size_t idx)
{
    if (idx > columns.size())
    {
        log::global->error("Trying to insert a new column for panel mode into wrong index");
        return;
    }
    
    auto it = columns.begin();
    std::advance(it, idx);
    columns.insert(it, col);
    rebuild();
}

void view::panel_mode_t::rebuild()
{
    titles.clear();
    std::vector<const wchar_t*> all_types, all_widths;

    for (const auto &column: columns)
    {
        titles.push_back(column.title);
        all_types.push_back(column.uid);
        all_widths.push_back(column.width);
    }

    if (columns.size() > 0)
    {
        types = utils::string_join(all_types, L",");
        widths = utils::string_join(all_widths, L",");
    }
}

void view::panel_modes_t::update()
{
    for (size_t i = 0; i < MODES_COUNT; i++)
    {
        auto &mode = at(i);

        // if the mode is empty, we check whether we need to reference to another one
        if (mode.is_empty() && mode.copy_of_idx >= 0)
            mode = at(mode.copy_of_idx);

        // filling up the Far mode struct in case we eventually found a valid object
        if (!mode.is_empty())
        {
            auto &far_mode = modes[i];

            far_mode.ColumnTypes = mode.types.c_str();
            far_mode.ColumnWidths = mode.widths.c_str();
            far_mode.ColumnTitles = &mode.titles[0];
            far_mode.Flags = mode.is_wide ? PMFLAGS_FULLSCREEN : PMFLAGS_NONE;
            far_mode.StatusColumnTypes = NULL;
            far_mode.StatusColumnWidths = NULL;
        }
    }
}

//-----------------------------------------------------------------------------------------------------
view::view(HANDLE panel, const wstring &title, const wstring &dir_name): panel(panel), title(title)
{
    this->dir_name = utils::strip_invalid_filename_chars(dir_name.empty() ? title : dir_name);
}

config::settings::view_t* view::get_settings() const
{
    return config::get_view_settings(get_uid(), get_default_settings());
}

void view::select_sort_mode(int idx)
{
    if (idx > (int)sort_modes.size())
    {
        log::global->error("Given sort mode index is out of range, index {}, "
            "view uid {}, modes count {}", idx, get_uid(), sort_modes.size());
        return;
    }

    const auto &sm = sort_modes[idx];

    if (idx == settings->sort_mode_idx)
        // if the sort mode index is the same as the current one - we invert
        // the sorting direction
        settings->is_descending = !settings->is_descending;
    else
        // otherwise, just change the sort mode
        settings->sort_mode_idx = idx;
    
    panels::set_sort_mode(get_panel_handle(), sm.far_sort_mode, settings->is_descending);
}

void view::on_items_updated()
{
    if (is_first_init)
    {
        is_first_init = false;

        settings = get_settings();
        sort_modes = get_sort_modes();
        
        // settings a stored sort mode
        if ((int)sort_modes.size() > settings->sort_mode_idx)
            panels::set_sort_mode(get_panel_handle(),
                sort_modes[settings->sort_mode_idx].far_sort_mode, settings->is_descending);

        // settings a stored view mode
        panels::set_view_mode(get_panel_handle(), settings->view_mode);
    }
}

const data_item_t* view::unpack_user_data(const UserDataItem &user_data)
{
    if (user_data.Data != nullptr)
        return reinterpret_cast<const data_item_t*>(user_data.Data);
    return nullptr;
}

intptr_t view::compare_items(const CompareInfo *info)
{
    if ((int)sort_modes.size() > settings->sort_mode_idx)
        return compare_items(
            sort_modes[settings->sort_mode_idx],
            unpack_user_data(info->Item1->UserData),
            unpack_user_data(info->Item2->UserData)
        );
    return -2; // fallback solution - delegate sorting to Far
}

item_ids_t view::get_selected_items()
{
    item_ids_t result;
    const auto &items = panels::get_items(get_panel_handle(), true);
    for (size_t idx = 0; idx < items.size(); idx++)
    {
        if (auto user_data = unpack_user_data(items[idx]->UserData))
            result.push_back(user_data->id);
    }
    return result;
}

intptr_t view::process_input(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        auto key = keys::make_combined(key_event);
        
        for (size_t idx = 0; idx < sort_modes.size(); idx++)
        {
            const auto &smode = sort_modes[idx];
            if (key == smode.get_combined_key())
            {
                select_sort_mode((int)idx);
                return TRUE;
            }
        }
        
        // all the sorting hotkeys Ctrl+(F3...F12) are blocked, due to custom plugin implementation
        for (int key_code = VK_F3; key_code <= VK_F12; key_code++)
            if (key == key_code + keys::mods::ctrl)
                return TRUE;
        
        // view mode hotkeys Ctrl+(1...0) pre-handling: saving the index into
        // specific view persistent settings to recover later
        for (int key_code = keys::key_0; key_code <= keys::key_9; key_code++)
            if (key == key_code + keys::mods::ctrl)
            {
                settings->view_mode = key_code - keys::key_0;
                return TRUE;
            }

        return process_key_input(key);
    }
    return FALSE;
}

intptr_t view::select_item(const SetDirectoryInfo *info)
{
    if (info->UserData.Data == nullptr)
    {
        if (return_callback)
        {
            return_callback();
            return TRUE;
        }

        // we should've change view, but didn't. No panel update is required
        return FALSE;
    }

    return select_item(unpack_user_data(info->UserData));
}

void view::update_panel_info(OpenPanelInfo *info)
{
    if (const auto pmodes = get_panel_modes())
    {
        info->PanelModesArray = pmodes->get_modes();
        info->PanelModesNumber = pmodes->size();
    }
}

intptr_t view::request_extra_info(const PluginPanelItem *data)
{
    return request_extra_info(unpack_user_data(data->UserData));
}

size_t view::get_item_idx(const string &item_id)
{
    const auto &items = panels::get_items(get_panel_handle());
    for (size_t idx = 1; idx < items.size(); idx++) // zero index is ".." folder
    {
        auto user_data = unpack_user_data(items[idx]->UserData);
        if (user_data != nullptr && user_data->id == item_id)
           return idx;
    }
    return 0;
}

} // namespace ui
} // namespace spotifar