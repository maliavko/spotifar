#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief A base class for all the views, representing a list
/// of artist in either way
class artists_base_view: public view_abstract
{
public:
    artists_base_view(api_abstract *api, const string &view_uid,
        const wstring &title, return_callback_t callback);

    auto get_items() -> const items_t*;
protected:
    virtual auto get_artists() -> std::generator<const artist_t&> = 0;
    virtual void show_albums_view(const artist_t &artist) const = 0;

    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto select_item(const data_item_t *data) -> intptr_t override;
    auto update_panel_info(OpenPanelInfo *info) -> void override;
    auto request_extra_info(const data_item_t *data) -> bool override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
protected:
    api_abstract *api_proxy;
};

/// @brief A class-view, representing a list of followed artists
class followed_artists_view: public artists_base_view
{
public:
    followed_artists_view(api_abstract *api);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    
    // artists_base_view
    auto get_artists() -> std::generator<const artist_t&> override;
    void show_albums_view(const artist_t &artist) const override;
private:
    followed_artists_ptr collection;
};

/// @brief A class-view, representing a list of recently listened
/// artists. A spotify API does not support such data, so the logic inside
/// takes a list of recently listened tracks and calculate their artists
class recent_artists_view:
    public artists_base_view,
    public play_history_observer
{
public:
    struct history_artist_t: public artist_t
    {
        string played_at;
    };
public:
    recent_artists_view(api_abstract *api);
    ~recent_artists_view();
protected:
    auto rebuild_items() -> void;

    // view interface
    auto get_sort_modes() const -> const view_abstract::sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
    
    // artists_base_view
    auto get_artists() -> std::generator<const artist_t&> override;
    auto show_albums_view(const artist_t &artist) const -> void override;
    
    void on_items_changed();
private:
    std::vector<history_artist_t> items;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0