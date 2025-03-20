#ifndef ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"
#include "spotify/history.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class albums_base_view: public view
{
public:
    albums_base_view(api_abstract *api, const string &view_uid,
        return_callback_t callback);

    auto get_items() -> const items_t*;
    auto get_sort_modes() const -> const sort_modes_t&;
    auto select_item(const data_item_t* data) -> intptr_t;
    auto request_extra_info(const data_item_t* data) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto process_key_input(int combined_key) -> intptr_t;
protected:
    auto compare_items(const sort_mode_t &sort_mode, const data_item_t *data1,
        const data_item_t *data2) -> intptr_t;
    
    virtual auto get_albums() -> std::generator<const simplified_album_t&> = 0;
    virtual auto show_tracks_view(const album_t &album) const -> void = 0;
protected:
    api_abstract *api_proxy;
};


/// @brief Showing the list of the given `artist` albums
class artist_view: public albums_base_view
{
public:
    artist_view(api_abstract *api, const artist_t &artist);
    
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto get_albums() -> std::generator<const simplified_album_t&>;
    auto show_tracks_view(const album_t &album) const -> void;
private:
    artist_t artist;
};


/// @brief Showing the list of the user's saved albums. Differes
/// from the standard one with the additional implementatino of the
/// `added_at` data, extending custom columns and sorting modes respectively
class albums_collection_view: public albums_base_view
{
public:
    albums_collection_view(api_abstract *api);

    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
    auto get_sort_modes() const -> const sort_modes_t&;
protected:
    auto compare_items(const sort_mode_t &sort_mode,
        const data_item_t *data1, const data_item_t *data2) -> intptr_t;
    
    auto get_albums() -> std::generator<const simplified_album_t&>;
    auto show_tracks_view(const album_t &album) const -> void;
};


/// @brief Showing the list of the newely released albums of the
/// followed artists.
/// TODO: not true, an API returns some trash of the all recently releases
/// albums on the platform
class new_releases_view: public albums_base_view
{
public:
    new_releases_view(api_abstract *api);

    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto compare_items(const sort_mode_t &sort_mode,
        const data_item_t *data1, const data_item_t *data2) -> intptr_t;
    
    auto get_albums() -> std::generator<const simplified_album_t&>;
    auto show_tracks_view(const album_t &album) const -> void;
};


class recent_albums_view:
    public albums_base_view,
    public play_history_observer
{
public:
    struct history_album_t: public album_t
    {
        string played_at;

        history_album_t(const string &played_at, const album_t &album):
            album_t(album), played_at(played_at)
            {}
    };
public:
    recent_albums_view(api_abstract *api);
    ~recent_albums_view();

    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
    auto get_sort_modes() const -> const view::sort_modes_t&;
protected:
    auto rebuild_items() -> void;

    auto compare_items(const sort_mode_t &sort_mode,
        const data_item_t *data1, const data_item_t *data2) -> intptr_t;
    auto get_albums() -> std::generator<const simplified_album_t&>;
    auto show_tracks_view(const album_t &album) const -> void;
    
    void on_items_changed();
private:
    std::vector<history_album_t> items;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61