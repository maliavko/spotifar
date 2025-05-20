#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "stdafx.h"
#include "abstract.hpp"
#include "spotify/common.hpp"
#include "ui/views/view.hpp" // view_abstract
#include "ui/events.hpp" // ui_events_observer

namespace spotifar { namespace ui {

using namespace spotify;

class panel:
    public ui_events_observer, // for processing view opening requests
    public api_requests_observer // for showing splash screen during long httl requests
{
public:
    panel(api_proxy_ptr api, std::shared_ptr<plugin_interface> p);
    ~panel();

    // Far API interface
    void update_panel_info(OpenPanelInfo *info);
    void free_panel_items(const FreeFindDataInfo *info);
    intptr_t update_panel_items(GetFindDataInfo *info);
    intptr_t select_directory(const SetDirectoryInfo *info);
    intptr_t process_input(const ProcessPanelInputInfo *info);
    intptr_t compare_items(const CompareInfo *info);
protected:
    void show_stub_view();
    
    // global ui events
    void refresh_panels(const item_id_t &item_id = "") override;
    void on_show_filters_menu() override;
    void show_view(view_ptr view) override;
    void show_fildered_view(ui_events_observer::view_filter_callbacks callbacks) override;

    // requesters progress notifications
    void on_request_started(const string &url) override;
    void on_request_finished(const string &url) override;
    void on_request_progress_changed(const string &url, size_t progress, size_t total) override;
    void on_playback_command_failed(const string &message) override;
    void on_collection_fetching_failed(const string &message) override;
private:
    ui_events_observer::view_filter_callbacks filter_callbacks;
    size_t current_filter_idx = 0;

    view_ptr view;
    api_proxy_ptr api_proxy;
    std::shared_ptr<plugin_interface> plugin_proxy;
};

} // namespace ui
} // namespace spotifar

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B