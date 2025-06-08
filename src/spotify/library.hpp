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

    bool is_track_saved(const item_id_t &id);
protected:
    // json_cache's interface
    bool is_active() const override;
    bool request_data(data_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
    void on_data_synced(const data_t &data, const data_t &prev_data) override;
private:
    api_interface *api_proxy;
    item_ids_t tracks_to_process;
    std::mutex data_access_guard;
};

struct collection_observer: public BaseObserverProtocol
{
    virtual void on_saved_tracks_status_received(const library_statuses_t &saved_status) {}
};

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465