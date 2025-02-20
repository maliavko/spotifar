#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "spotify/abstract.hpp"
#include "ui/views/view.hpp"
#include "ui/events.hpp" // ui_events_observer

namespace spotifar { namespace ui {

using namespace spotify;

class Panel: public ui_events_observer
{
public:
    Panel(spotify::api_abstract *api);
    virtual ~Panel();
    // TODO: consider having here shutdown/close method to cleanup resources

    // far global events' handlers
    void update_panel_info(OpenPanelInfo *info);
    intptr_t update_panel_items(GetFindDataInfo *info);
    void free_panel_items(const FreeFindDataInfo *info);
    intptr_t select_item(const SetDirectoryInfo *info);
    intptr_t process_input(const ProcessPanelInputInfo *info);
protected:
    static void WINAPI free_user_data(void *const user_data, const FarPanelItemFreeInfo *const info);
    void change_view(std::shared_ptr<ui::view> view);

    // views events' handlers
    virtual void refresh_panels(const string &item_id);
    virtual void show_root_view();
    virtual void show_artists_view();
    virtual void show_artist_view(const artist &artist);
    virtual void show_album_view(const artist &artist, const album &album);
    virtual void show_playlists_view();
    virtual void show_playlist_view(const playlist &playlist);
private:
    std::shared_ptr<ui::view> view;
    api_abstract *api_proxy;
};

} // namespace ui
} // namespace spotifar

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B