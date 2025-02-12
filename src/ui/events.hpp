#ifndef EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#define EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3
#pragma once

#include "stdafx.h"
#include "spotify/items.hpp"

namespace spotifar { namespace ui {

struct ui_events_observer: public BaseObserverProtocol
{
    virtual void show_root_view() {}
    virtual void show_artists_view() {}
    virtual void show_artist_view(const spotify::artist &artist) {}
    virtual void show_album_view(const spotify::artist &artist, const spotify::album &album) {}
    virtual void show_playlists_view() {}
    virtual void show_player_dialog() {}
};

namespace events {

    void show_root_view();

    void show_artists_view();

    void show_playlists_view();

    void show_artist_view(const spotify::artist &artist);

    void show_album_view(const spotify::artist &artist, const spotify::album &album);
    
    void show_player_dialog();
    
    void show_config_dialog();

} // namespace events

} // namespace ui
} // namespace spotiar

#endif // EVENTS_HPP_4C924B6D_3982_4D12_A289_7D2FAADDCBD3