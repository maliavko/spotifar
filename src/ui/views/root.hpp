#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class root_base_view: public view
{
public:
    struct root_data_t: public data_item_t
    {
        int name_key;
        int descr_key;
    };

    typedef std::vector<root_data_t> menu_items_t;
public:
    root_base_view(api_abstract *api, const string &uid, return_callback_t callback,
        menu_items_t items);

    auto get_items() -> const items_t*;
    auto get_key_bar_info() -> const key_bar_info_t*;
    auto get_info_lines() -> const info_lines_t*;
protected:
    auto get_sort_modes() const -> const sort_modes_t& override;
    auto get_default_settings() const -> config::settings::view_t override;
    auto update_panel_info(OpenPanelInfo *info) -> void override;
    auto request_extra_info(const data_item_t* data) -> bool override;

    virtual auto get_total(const string &menu_id, bool only_cached) -> size_t { return 0; }
protected:
    const menu_items_t menu_items;
    api_abstract *api_proxy;
};

class root_view: public root_base_view
{
public:
    inline static const string
        collection_id = "collection",
        browse_id = "browse",
        recents_id = "recents";
public:
    root_view(api_abstract *api);

    auto get_dir_name() const -> const wstring& override;
protected:
    auto select_item(const data_item_t *data) -> intptr_t override;
};

class recents_view: public root_base_view
{
public:
    inline static const string
        tracks_id = "tracks",
        artists_id = "artists",
        albums_id = "albums",
        playlists_id = "playlists";
public:
    recents_view(api_abstract *api);

    auto get_dir_name() const -> const wstring& override;
protected:
    auto select_item(const data_item_t *data) -> intptr_t override;
};

class collection_view: public root_base_view
{
public:
    inline static const string
        artists_id = "artists",
        albums_id = "albums",
        tracks_id = "tracks",
        playlists_id = "playlists";
public:
    collection_view(api_abstract *api);

    auto get_dir_name() const -> const wstring& override;
protected:
    auto select_item(const data_item_t *data) -> intptr_t override;
    auto get_total(const string &menu_id, bool only_cached) -> size_t override;
};

} // namespace ui
} // namespace spotifar

#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E