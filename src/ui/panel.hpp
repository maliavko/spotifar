#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "stdafx.h"
#include "abstract.hpp"
#include "spotify/common.hpp"
#include "ui/views/view.hpp" // view
#include "ui/events.hpp" // ui_events_observer

namespace spotifar { namespace ui {

using namespace spotify;

class panel:
    public ui_events_observer
{
public:
    panel(plugin_ptr_t plugin_ptr);
    virtual ~panel();

    // Far API interface
    void update_panel_info(OpenPanelInfo *info);
    void free_panel_items(const FreeFindDataInfo *info);
    intptr_t update_panel_items(GetFindDataInfo *info);
    intptr_t select_directory(const SetDirectoryInfo *info);
    intptr_t process_input(const ProcessPanelInputInfo *info);
    intptr_t compare_items(const CompareInfo *info);
protected:
    void set_view(view_ptr_t, view::return_callback_t = {});
    bool is_active() const;
    bool is_this_panel(HANDLE panel) const;
    void refresh(const string &item_id = "");
    
    // global ui events
    void refresh_panels(HANDLE panel, const spotify::item_id_t &item_id = "") override;
    void show_view(HANDLE panel, view_builder_t, view::return_callback_t = nullptr) override;
    void show_multiview(HANDLE panel, multiview_builder_t, view::return_callback_t = nullptr) override;
    void close_panel(HANDLE panel) override;
    void show_filters_menu() override;
private:
    multiview_builder_t mview_builders;
    bool skip_view_refresh = true;

    view_ptr_t view;
    plugin_ptr_t plugin_proxy;
};

} // namespace ui
} // namespace spotifar

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B