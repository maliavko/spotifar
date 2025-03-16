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
    albums_base_view(api_abstract *api, const string &view_uid):
        view(view_uid), api_proxy(api)
        {}

    auto get_items() -> const items_t*;
    auto get_sort_modes() const -> const sort_modes_t&;
    auto select_item(const spotify::data_item* data) -> intptr_t;
    auto request_extra_info(const spotify::data_item* data) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto process_key_input(int combined_key) -> intptr_t;
protected:
    auto compare_items(const sort_mode_t &sort_mode, const spotify::data_item *data1,
        const spotify::data_item *data2) -> intptr_t;
    
    virtual auto get_albums() -> std::generator<const simplified_album&> = 0;
    virtual auto goto_root_folder() -> void = 0;
protected:
    api_abstract *api_proxy;
};


/// @brief Showing the list of the given `artist` albums.
class artist_view: public albums_base_view
{
public:
    artist_view(api_abstract *api, const artist &artist);
    
    auto get_default_settings() const -> config::settings::view_t;
    auto get_dir_name() const -> const wstring&;
protected:
    auto goto_root_folder() -> void;
    auto get_albums() -> std::generator<const simplified_album&>;
private:
    artist artist;
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
    auto goto_root_folder() -> void;
    auto get_albums() -> std::generator<const simplified_album&>;
    auto compare_items(const sort_mode_t &sort_mode,
        const spotify::data_item *data1, const spotify::data_item *data2) -> intptr_t;
};

} // namespace ui
} // namespace spotifar

#endif // ALBUMS_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61