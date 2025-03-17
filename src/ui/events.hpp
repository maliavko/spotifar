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
    virtual void show_player() {}
    virtual void refresh_panels(const string &item_id) {}
    virtual void show_panel_view(std::shared_ptr<ui::view> view) {}
};

namespace events {

    void show_root(api_abstract *api);

    void show_collections(api_abstract *api);

    void show_artists_collection(api_abstract *api);

    void show_albums_collection(api_abstract *api);

    void show_tracks_collection(api_abstract *api);

    void show_playlists(api_abstract *api);

    void show_recents(api_abstract *api);

    void show_browse(api_abstract *api);

    void show_playlist(api_abstract *api, const playlist_t &playlist);

    void show_artist(api_abstract *api, const artist_t &artist);

    void show_album_tracks(api_abstract *api, const album_t &album,
        view::return_callback_t callback = {});
    
    void show_player();
    
    void show_config();

    void refresh_panels(const string &item_id = "");

} // namespace events

} // namespace ui
} // namespace spotiar

#endif // EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3