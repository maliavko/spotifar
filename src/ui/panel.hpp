#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "spotify/api.hpp"
#include "ui/views/view.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

class Panel: public ui_events_observer
{
public:
    Panel(spotify::api &api);
    virtual ~Panel();
    // TODO: consider having here shutdown/close method to cleanup resources

    // far global events' handlers
    void update_panel_info(OpenPanelInfo *info);
    intptr_t update_panel_items(GetFindDataInfo *info);
    void free_panel_items(const FreeFindDataInfo *info);
    intptr_t select_item(const SetDirectoryInfo *info);
    intptr_t process_input(const ProcessPanelInputInfo *info);
protected:
    static void WINAPI free_user_data(void *const UserData, const FarPanelItemFreeInfo *const Info);
    void change_view(std::shared_ptr<ui::view> view);

    // views events' handlers
    virtual void show_root_view();
    virtual void show_artists_view();
    virtual void show_artist_view(const spotify::artist &artist);
    virtual void show_album_view(const spotify::artist &artist, const spotify::album &album);
    virtual void show_playlists_view();
private:
    std::shared_ptr<ui::view> view;
    spotify::api &api;
};

} // namespace ui
} // namespace spotifar

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B