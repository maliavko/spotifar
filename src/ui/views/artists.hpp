#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/common.hpp"
#include "spotify/history.hpp"
#include "spotify/collection.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief A base class for all the views, representing a list
/// of artists in either way
class artists_base_view:
    public view,
    public collection_observer
{
public:
    artists_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name = L"");
    ~artists_base_view();

    auto get_items() -> const items_t& override;
protected:
    virtual auto get_artists() -> std::generator<const artist_t&> = 0;
    virtual void show_albums_view(const artist_t &artist) const = 0;

    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto select_item(const data_item_t *data) -> intptr_t override;
    bool request_extra_info(const data_item_t *data) override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
    auto get_panel_modes() const -> const panel_modes_t* override;
    auto process_key_input(int combined_key) -> intptr_t override;
    auto get_key_bar_info() -> const key_bar_info_t* override;

    // collection_observer
    void on_artists_statuses_changed(const item_ids_t &ids) override;
    void on_artists_statuses_received(const item_ids_t &ids) override;
protected:
    api_weak_ptr_t api_proxy;
    items_t items;
};


/// TODO: once the artists are added to the collection worker,
/// subscribe to the appropriate events and rebuild items on changing
/// @brief A class-view, representing a list of followed artists
class followed_artists_view: public artists_base_view
{
public:
    followed_artists_view(HANDLE panel, api_weak_ptr_t api);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;
    
    // artists_base_view interface
    auto get_artists() -> std::generator<const artist_t&> override;
    void show_albums_view(const artist_t &artist) const override;
    void show_filters_dialog() override;
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
    recent_artists_view(HANDLE panel, api_weak_ptr_t api);
    ~recent_artists_view();
protected:
    void rebuild_items();

    // view interface
    auto get_sort_modes() const -> const view::sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
    
    // artists_base_view
    auto get_artists() -> std::generator<const artist_t&> override;
    void show_albums_view(const artist_t &artist) const override;
    
    void on_history_changed() override;
private:
    std::vector<history_artist_t> items;
};


/// @brief
class user_top_artists_view: public artists_base_view
{
public:
    user_top_artists_view(HANDLE panel, api_weak_ptr_t api);
protected:
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    
    // artists_base_view
    auto get_artists() -> std::generator<const artist_t&> override;
    void show_albums_view(const artist_t &artist) const override;
private:
    user_top_artists_ptr collection;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0