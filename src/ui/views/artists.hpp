#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artists_base_view: public view
{
public:
    artists_base_view(api_abstract *api, const string &view_uid,
        return_callback_t callback);

    auto get_items() -> const items_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t&;
    auto select_item(const data_item_t *data) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto request_extra_info(const data_item_t *data) -> bool;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;
    
    virtual auto get_artists() -> std::generator<const artist_t&> = 0;
    virtual auto show_albums_view(const artist_t &artist) const -> void = 0;
protected:
    api_abstract *api_proxy;
};


class followed_artists_view: public artists_base_view
{
public:
    followed_artists_view(api_abstract *api);

    auto get_dir_name() const -> const wstring&;
protected:
    auto get_default_settings() const -> config::settings::view_t;
    auto get_artists() -> std::generator<const artist_t&>;
    auto show_albums_view(const artist_t &artist) const -> void;
};


class recent_artists_view:
    public artists_base_view,
    public play_history_observer
{
public:
    struct history_artist_t: public artist_t
    {
        string played_at;

        history_artist_t(const string &played_at, const artist_t &artist):
            artist_t(artist), played_at(played_at)
            {}
    };
public:
    recent_artists_view(api_abstract *api);
    ~recent_artists_view();

    auto get_dir_name() const -> const wstring&;
protected:
    auto rebuild_items() -> void;

    auto get_sort_modes() const -> const view::sort_modes_t&;
    auto get_default_settings() const -> config::settings::view_t;
    auto compare_items(const sort_mode_t &sort_mode,
        const data_item_t *data1, const data_item_t *data2) -> intptr_t;
    auto get_artists() -> std::generator<const artist_t&>;
    auto show_albums_view(const artist_t &artist) const -> void;
    
    void on_items_changed();
private:
    std::vector<history_artist_t> items;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0