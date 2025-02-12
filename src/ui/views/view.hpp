#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"
#include "utils.hpp"

namespace spotifar { namespace ui {

struct view_item
{
    string id;
    wstring name;
    wstring description;
    uintptr_t file_attrs;
    size_t duration;
    bool is_current;

    view_item(const string &id, const wstring &name, const wstring &descr,
        uintptr_t attrs = 0, size_t duration = 0, bool is_current = false);
};

class view
{
public:
    typedef std::vector<view_item> view_items_t;
public:
    view(const wstring &name): name(utils::strip_invalid_filename_chars(name)) {}
    virtual ~view() {}

    inline const wstring& get_name() const { return name; }

    virtual void update_panel_info(OpenPanelInfo *info) {}
    virtual intptr_t process_input(const ProcessPanelInputInfo *info) { return FALSE; }
    virtual view_items_t get_items() = 0;
    virtual intptr_t select_item(const string &id) = 0;

protected:
    const wstring name;
};

} // namespace ui
} // namespace spotifar

#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467