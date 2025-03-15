#ifndef ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class albums_base_view: public view
{
public:
    struct album_user_data_t: public user_data_t
    {
        string release_year;
        size_t tracks_total;
    };
public:
    albums_base_view(api_abstract *api, const string &view_uid):
        view(view_uid), api_proxy(api)
        {}

    auto get_sort_modes() const -> const sort_modes_t&;
    auto select_item(const user_data_t* data) -> intptr_t;
    auto request_extra_info(const user_data_t* data) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto process_key_input(int combined_key) -> intptr_t;
protected:
    auto compare_items(const sort_mode_t &sort_mode, const user_data_t *data1,
        const user_data_t *data2) -> intptr_t;
    
    virtual auto goto_root_folder() -> void = 0;
    virtual auto pack_custom_columns(std::vector<wstring> &columns, const simplified_album &a) -> void;
protected:
    api_abstract *api_proxy;
};


/// @brief Showing the list of the given `artist` albums.
class artist_view: public albums_base_view
{
public:
    artist_view(api_abstract *api, const artist &artist);
    
    auto get_items() -> const items_t*;
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto goto_root_folder() -> void;
private:
    artist artist;
};


/// @brief Showing the list of the user's saved albums. Differes
/// from the standard one with the additional implementatino of the
/// `added_at` data, extending custom columns and sorting modes respectively
class albums_collection_view: public albums_base_view
{
public:
    struct saved_album_user_data_t: public album_user_data_t
    {
        string added_at;
    };
public:
    albums_collection_view(api_abstract *api);

    auto get_items() -> const items_t*;
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
    auto get_sort_modes() const -> const sort_modes_t&;
protected:
    auto goto_root_folder() -> void;
    auto compare_items(const sort_mode_t &sort_mode,
        const user_data_t *data1, const user_data_t *data2) -> intptr_t;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61