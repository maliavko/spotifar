#ifndef ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/common.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

/// @brief A base class for all the view representing a list
/// of albums in either way
class albums_base_view: public view_abstract
{
public:
    albums_base_view(api_proxy_ptr api, const string &view_uid,
        const wstring &title, return_callback_t callback);
    ~albums_base_view() { api_proxy.reset(); }

    auto get_items() -> const items_t& override;
protected:
    virtual auto get_albums() -> std::generator<const simplified_album_t&> = 0;
    virtual void show_tracks_view(const album_t &album) const = 0;

    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto select_item(const data_item_t* data) -> intptr_t override;
    auto request_extra_info(const data_item_t* data) -> bool override;
    auto update_panel_info(OpenPanelInfo *info) -> void override;
    auto process_key_input(int combined_key) -> intptr_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;
protected:
    api_proxy_ptr api_proxy;
};

/// @brief A class-view, representing the list of albums of a
/// given `artist`
class artist_view: public albums_base_view
{
public:
    artist_view(api_proxy_ptr api, const artist_t &artist,
        return_callback_t callback);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const simplified_album_t&> override;
    auto show_tracks_view(const album_t &album) const -> void override;
private:
    artist_t artist;
    artist_albums_ptr collection;
};

/// @brief Showing the list of the user's saved albums. Differes
/// from the standard one with the additional implementatino of the
/// `added_at` data, extending custom columns and sorting modes respectively
class saved_albums_view: public albums_base_view
{
public:
    saved_albums_view(api_proxy_ptr api);
protected:
    // view interface
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const simplified_album_t&> override;
    auto show_tracks_view(const album_t &album) const -> void override;
private:
    saved_albums_ptr collection;
};

/// @brief Showing the list of the newely released albums of the
/// followed artists.
/// TODO: not true, an API returns some trash of the all recently releases
/// albums on the platform
class new_releases_view: public albums_base_view
{
public:
    new_releases_view(api_proxy_ptr api);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const simplified_album_t&> override;
    auto show_tracks_view(const album_t &album) const -> void override;
private:
    new_releases_ptr collection;
};

/// @brief A class-view representing a recently played albums. A spotify
/// API does not have anything suitable, so in basic we use a list of recently
/// played tracks and accumulate the list of these tracks' albums, sorting
/// by `played_at` attribute by default
class recent_albums_view:
    public albums_base_view,
    public play_history_observer // for updating the view each time the currently played tracks is changed
{
public:
    struct history_album_t: public album_t
    {
        string played_at;
    };
public:
    recent_albums_view(api_proxy_ptr api);
    ~recent_albums_view();
protected:
    void rebuild_items();

    // view interface
    auto get_sort_modes() const -> const view_abstract::sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1, const data_item_t *data2) -> intptr_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const simplified_album_t&> override;
    auto show_tracks_view(const album_t &album) const -> void override;

    // play_history_observer interface
    void on_items_changed() override;
private:
    std::vector<history_album_t> items;
};

/// @brief A view class representing list of albums, featuring tracks
/// which user liked reecntly
class featuring_albums_view: public albums_base_view
{
public:
    featuring_albums_view(api_proxy_ptr api);
protected:
    // view interface
    auto get_default_settings() const -> config::settings::view_t override;

    // albums_base_view interface
    auto get_albums() -> std::generator<const simplified_album_t&> override;
    auto show_tracks_view(const album_t &album) const -> void override;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61