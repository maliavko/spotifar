#ifndef EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#define EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#pragma once

#include "stdafx.h"
#include "spotify/items.hpp"
#include "spotify/abstract.hpp"
#include "ui/views/view.hpp"

namespace spotifar { namespace ui {
    
using namespace spotify;

struct ui_events_observer: public BaseObserverProtocol
{
    virtual void show_player_dialog() {}
    virtual void refresh_panels(const string &item_id) {}
    virtual void show_panel_view(std::shared_ptr<ui::view> view) {}
};

namespace events {

    void show_root_view(api_abstract *api);

    void show_collection_view(api_abstract *api);
    void show_albums_collection_view(api_abstract *api);

    void show_recents_view(api_abstract *api);

    void show_browse_view(api_abstract *api);

    void show_artists_view(api_abstract *api);

    void show_tracks_view(api_abstract *api);

    void show_playlists_view(api_abstract *api);

    void show_playlist_view(api_abstract *api, const playlist &playlist);

    void show_artist_view(api_abstract *api, const artist &artist);

    void show_album_view(api_abstract *api, const album &album);
    
    void show_player_dialog();
    
    void show_config_dialog();

    void refresh_panels(const string &item_id = "");

} // namespace events

} // namespace ui
} // namespace spotiar

#endif // EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3