#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/abstract.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class root_view: public view
{
public:
    root_view(api_abstract *api);

    auto get_dir_name() const -> const wchar_t*;
    auto get_title() const -> const wchar_t*;
    auto get_items() -> const items_t*;
    auto get_key_bar_info() -> const key_bar_info_t*;
    auto get_info_lines() -> const info_lines_t*;

    auto select_item(const string &view_id) -> intptr_t;
    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto request_extra_info(const string &view_id) -> bool;
protected:
    template<class T>
    auto pack_menu_item(const string &id, int name_msg_id, int descr_msg_id,
        spotify::api_collection_requester<T> &&req) -> view::item_t;
private:
    api_abstract *api_proxy;
};

template<class T>
view::item_t root_view::pack_menu_item(const string &id, int name_msg_id, int descr_msg_id,
    spotify::api_collection_requester<T> &&req)
{
    // column C0 - total count of entries in menu
    wstring entries_count = L"";
    if (api_proxy->is_request_cached(req.get_url()) && req(api_proxy))
        entries_count = std::format(L"{: >6}", (req.get_total()));

    return {
        id,
        utils::far3::get_text(name_msg_id),
        utils::far3::get_text(descr_msg_id),
        FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
        0,
        {
            entries_count
        }
    };
}

} // namespace ui
} // namespace spotifar

#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E