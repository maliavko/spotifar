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

class library: public json_cache<saved_items_t>
{
public:
    library(api_interface *api): json_cache(), api_proxy(api) {}
    ~library() { api_proxy = nullptr; }

    /// @brief https://developer.spotify.com/documentation/web-api/reference/check-users-saved-tracks 
    bool is_track_saved(const item_id_t &id, bool force_sync = false);

    // https://developer.spotify.com/documentation/web-api/reference/save-tracks-user
    bool save_tracks(const item_ids_t &ids);

    // https://developer.spotify.com/documentation/web-api/reference/remove-tracks-user
    bool remove_saved_tracks(const item_ids_t &ids);
protected:
    auto check_saved_tracks(const item_ids_t &ids) -> std::deque<bool>;

    // json_cache's interface
    bool is_active() const override;
    bool request_data(data_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
private:
    api_interface *api_proxy;
    item_ids_t tracks_to_process;
    std::mutex data_access_guard;
};

struct collection_observer: public BaseObserverProtocol
{
    virtual void on_saved_tracks_status_received(const library_statuses_t &changed_tracks) {}
};

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465