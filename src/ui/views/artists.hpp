#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class artists_view: public view
{
public:
    struct artist_user_data_t: public view::user_data_t
    {
        size_t popularity;
        size_t followers_count;
        
        static void WINAPI free(void *const user_data, const FarPanelItemFreeInfo *const info)
        {
            delete reinterpret_cast<const artist_user_data_t*>(user_data);
        }
        
        static const artist_user_data_t* unpack(const UserDataItem &user_data)
        {
            if (user_data.Data != nullptr)
                return reinterpret_cast<const artist_user_data_t*>(user_data.Data);
            return nullptr;
        }
    };

public:
    artists_view(api_abstract *api);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;

    auto select_item(const SetDirectoryInfo *info) -> intptr_t;
    auto request_extra_info(const PluginPanelItem *item) -> bool;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto compare_items(const CompareInfo *info) -> intptr_t;
    auto process_key_input(int combined_key) -> intptr_t ;
private:
    api_abstract *api_proxy;
    size_t sort_mode = 1;
    bool is_desc = false;
};

} // namespace ui
} // namespace spotifar

#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0