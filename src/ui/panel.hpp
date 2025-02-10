#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "spotify/api.hpp"
#include "ui/views/view.hpp"

namespace spotifar { namespace ui {

class Panel
{
public:
    Panel(spotify::api &api);
    virtual ~Panel();
    // TODO: consider having here shutdown/close method to cleanup resources
    
    void gotoRootMenu();

    void update_panel_info(OpenPanelInfo *info);
    intptr_t update_panel_items(GetFindDataInfo *info);
    void free_panel_items(const FreeFindDataInfo *info);
    intptr_t select_item(const SetDirectoryInfo *info);
    intptr_t process_input(const ProcessPanelInputInfo *info);
protected:
    static void WINAPI free_user_data(void *const UserData, const FarPanelItemFreeInfo *const Info);
private:
    std::shared_ptr<ui::view> view;
    spotify::api &api;
};

} // namespace ui
} // namespace spotifar

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B