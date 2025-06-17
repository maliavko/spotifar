#ifndef ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/interfaces.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief A base class for all the view representing a list
/// of albums in either way
class albums_base_view:
    public view,
    public collection_observer
{
public:
    albums_base_view(HANDLE panel, api_weak_ptr_t api, const wstring &title, const wstring &dir_name = L"");
    ~albums_base_view();

    auto get_items() -> const items_t& override;
protected:
    virtual auto get_albums() -> std::generator<const album_t&> = 0;
    virtual void show_tracks_view(const album_t &album) const = 0;
    virtual auto get_extra_columns(const album_t&) const -> std::vector<wstring> { return {}; }

    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto select_item(const data_item_t* data) -> intptr_t override;
    bool request_extra_info(const data_item_t* data) override;
    auto process_key_input(int combined_key) -> intptr_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
    auto get_key_bar_info() -> const key_bar_info_t* override;
    auto get_panel_modes() const -> const panel_modes_t* override;

    // collection_observer
    void on_albums_statuses_changed(const item_ids_t &ids) override;
    void on_albums_statuses_received(const item_ids_t &ids) override;
protected:
    api_weak_ptr_t api_proxy;
    items_t items;
};


/// @brief A class-view, representing the list of albums of a given `artist`
class artist_albums_view: public albums_base_view
{
public:
    artist_albums_view(HANDLE panel, api_weak_ptr_t api, const artist_t &artist);
    ~artist_albums_view() { albums.clear(); }
protected:
    void rebuild_items();

    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const album_t&> override;
    void show_tracks_view(const album_t &album) const override;
private:
    artist_t artist;
    std::vector<album_t> albums;
};


/// @brief Showing the list of the user's saved albums. Differes
/// from the standard one with the additional implementation of the
/// `added_at` data field, extending custom columns and sorting modes
/// respectively
class saved_albums_view: public albums_base_view
{
public:
    saved_albums_view(HANDLE panel, api_weak_ptr_t api);
protected:
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const album_t&> override;
    void show_tracks_view(const album_t &album) const override;
    auto get_extra_columns(const album_t&) const -> std::vector<wstring> override;
    auto get_panel_modes() const -> const panel_modes_t* override { return &panel_modes; }
    
    // @experimental: with this handler uncommented the view will be updating each time
    // we like or unlike tracks: repopulate and redraw, without - just redraw. So, the list
    // will stay the same, just refreshed the items' statuses
    // 
    // collection handlers
    //virtual void on_albums_statuses_changed(const item_ids_t &ids) override;
private:
    saved_albums_ptr collection;
    panel_modes_t panel_modes;
};


/// @brief Showing the list of the newely released albums of the
/// followed artists.
class new_releases_view:
    public albums_base_view,
    public releases_observer
{
public:
    new_releases_view(HANDLE panel, api_weak_ptr_t api);
    ~new_releases_view();
protected:
    void rebuild_items();

    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const album_t&> override;
    void show_tracks_view(const album_t &album) const override;

    // releases_observer interface
    void on_releases_sync_finished(const recent_releases_t releases) override;
private:
    std::vector<album_t> recent_releases;
};


/// @brief A class-view representing a recently played albums. A spotify
/// API does not have anything suitable, so in basic we use a list of recently
/// played tracks and accumulate the list of these tracks' albums, sorting
/// by `played_at` attribute by default
class recent_albums_view:
    public albums_base_view,
    public play_history_observer
{
public:
    struct history_album_t: public album_t
    {
        string played_at;
    };
public:
    recent_albums_view(HANDLE panel, api_weak_ptr_t api);
    ~recent_albums_view();
protected:
    void rebuild_items();

    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const album_t&> override;
    void show_tracks_view(const album_t &album) const override;

    // play_history_observer interface
    void on_history_changed() override;
private:
    std::vector<history_album_t> items;
};


/// @brief
class recently_saved_albums_view: public albums_base_view
{
public:
    recently_saved_albums_view(HANDLE panel, api_weak_ptr_t api);
protected:
    bool repopulate();
    
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const album_t&> override;
    void show_tracks_view(const album_t &album) const override;
    auto get_extra_columns(const album_t&) const -> std::vector<wstring> override;
    auto get_panel_modes() const -> const panel_modes_t* override { return &panel_modes; }
    
    // @experimental: with this handler uncommented the view will be updating each time
    // we like or unlike tracks: repopulate and redraw, without - just redraw. So, the list
    // will stay the same, just refreshed the items' statuses
    // 
    // collection handlers
    //virtual void on_albums_statuses_changed(const item_ids_t &ids) override;
private:
    saved_albums_ptr collection;
    panel_modes_t panel_modes;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61