#ifndef LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465
#define LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465
#pragma once

#include "stdafx.h"
#include "common.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

using library_statuses_t = std::unordered_map<item_id_t, bool>;

struct saved_items_t
{
    library_statuses_t tracks;
    library_statuses_t albums;
    
    friend void from_json(const json::Value &j, saved_items_t &v);
    friend void to_json(json::Value &j, const saved_items_t &v, json::Allocator &allocator);
};

struct saved_items_cache_t
{
    using check_handler_t = std::function<std::deque<bool>(const item_ids_t &ids)>;
    using container_getter_t = std::function<library_statuses_t&(void)>;

    saved_items_cache_t(container_getter_t getter, check_handler_t check_handler):
        container_getter(getter), check_handler(check_handler)
        {}

    bool resync(library_statuses_t&);

    void update_saved_items(const item_ids_t &ids, bool status);
    
    bool is_item_saved(const item_id_t &item_id, bool force_sync);

    container_getter_t container_getter;
    check_handler_t check_handler;
    item_ids_t ids_to_process;
    std::mutex access_guard;
};

class library: public json_cache<saved_items_t>
{
public:
    library(api_interface *api);
    ~library();

    /// @brief https://developer.spotify.com/documentation/web-api/reference/check-users-saved-tracks 
    bool is_track_saved(const item_id_t &id, bool force_sync = false);

    // https://developer.spotify.com/documentation/web-api/reference/save-tracks-user
    bool save_tracks(const item_ids_t &ids);

    // https://developer.spotify.com/documentation/web-api/reference/remove-tracks-user
    bool remove_saved_tracks(const item_ids_t &ids);

    bool is_album_saved(const item_id_t &id, bool force_sync = false);

    bool save_albums(const item_ids_t &ids);

    bool remove_saved_albums(const item_ids_t &ids);
protected:
    auto check_saved_tracks(const item_ids_t &ids) -> std::deque<bool>;

    auto check_saved_albums(const item_ids_t &ids) -> std::deque<bool>;

    // json_cache's interface
    bool is_active() const override;
    bool request_data(data_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
private:
    api_interface *api_proxy;

    saved_items_cache_t tracks;
    saved_items_cache_t albums;
};

struct collection_observer: public BaseObserverProtocol
{
    virtual void on_saved_tracks_changed(const item_ids_t &ids) {}

    virtual void on_saved_albums_changed(const item_ids_t &ids) {}
};

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465