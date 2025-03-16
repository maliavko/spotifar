#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "stdafx.h"
#include "spotify/abstract.hpp"
#include "ui/views/view.hpp"
#include "ui/events.hpp" // ui_events_observer

namespace spotifar { namespace ui {

using namespace spotify;

class panel: public ui_events_observer
{
public:
    panel(api_abstract *api);
    virtual ~panel();
    // TODO: consider having here shutdown/close method to cleanup resources

    // far global events' handlers
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto update_panel_items(GetFindDataInfo *info) -> intptr_t;
    auto free_panel_items(const FreeFindDataInfo *info) -> void;
    auto select_directory(const SetDirectoryInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto compare_items(const CompareInfo *info) -> intptr_t;
protected:
    void show_panel_view(std::shared_ptr<ui::view> view);
    void refresh_panels(const string &item_id = "");
private:
    std::shared_ptr<ui::view> view = nullptr;
    api_abstract *api_proxy = nullptr;
};

} // namespace ui
} // namespace spotifar

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B