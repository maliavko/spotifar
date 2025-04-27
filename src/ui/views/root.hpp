#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/common.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class root_base_view: public view_abstract
{
public:
    struct root_data_t: public data_item_t
    {
        int name_key;
        int descr_key;
        return_callback_t callback{};
    };

    using menu_items_t = std::vector<root_data_t>;
public:
    root_base_view(api_proxy_ptr api, const string &uid, const wstring &title,
        return_callback_t callback, menu_items_t items);
    ~root_base_view() { api_proxy.reset(); }

    auto get_items() -> const items_t& override;
    auto get_key_bar_info() -> const key_bar_info_t* override;
    auto get_info_lines() -> const info_lines_t* override;
protected:
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto update_panel_info(OpenPanelInfo *info) -> void override;
    auto request_extra_info(const data_item_t* data) -> bool override;
    auto select_item(const data_item_t *data) -> intptr_t override;

    virtual auto get_total(const item_id_t &menu_id, bool only_cached) -> size_t { return 0; }
protected:
    const menu_items_t menu_items;
    api_proxy_ptr api_proxy;
};

class root_view: public root_base_view
{
public:
    inline static const item_id_t
        collection_id = "collection",
        browse_id = "browse",
        recents_id = "recents";
public:
    root_view(api_proxy_ptr api);
};

class recents_view: public root_base_view
{
public:
    inline static const item_id_t
        tracks_id = "tracks",
        artists_id = "artists",
        albums_id = "albums",
        playlists_id = "playlists";
public:
    recents_view(api_proxy_ptr api);
};

class collection_view: public root_base_view
{
public:
    inline static const item_id_t
        artists_id = "artists",
        albums_id = "albums",
        tracks_id = "tracks",
        playlists_id = "playlists";
public:
    collection_view(api_proxy_ptr api);
protected:
    auto get_total(const item_id_t &menu_id, bool only_cached) -> size_t override;
};

class browse_view: public root_base_view
{
public:
    inline static const item_id_t
        new_releases_id = "new_releases",
        artists_featuring_likes_id = "artists_featuring_likes",
        albums_featuring_likes_id = "albums_featuring_likes";
public:
    browse_view(api_proxy_ptr api);
};

} // namespace ui
} // namespace spotifar

#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E