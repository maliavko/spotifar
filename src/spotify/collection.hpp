#ifndef collection_HPP_92081AAB_EE4E_40B2_814E_83B712132465
#define collection_HPP_92081AAB_EE4E_40B2_814E_83B712132465
#pragma once

#include "stdafx.h"
#include "common.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

/// @brief A container type for caching saving statuses for the Spotify API items.
/// true - saved, false - not saved, abcense - status is unknown
using statuses_container_t = std::unordered_map<item_id_t, bool>;

struct saved_items_t
{
    statuses_container_t tracks;
    statuses_container_t albums;
    
    friend void from_json(const json::Value &j, saved_items_t &v);
    friend void to_json(json::Value &j, const saved_items_t &v, json::Allocator &allocator);
};

using collection_base_t = json_cache<saved_items_t>;

/// @brief A cache container for the specific Spotify API items saving
/// statuses. Provides a mechanism for accessing, requesting and storing
/// the statuses with minimum overhead for the remote API
class saved_items_cache_t
{
    using data_accessor_t = std::function<collection_base_t::accessor_t()>;
public:
    /// @param accessor function-getter to obtain a main collection_base_t::data_t
    /// container for writing
    saved_items_cache_t(api_interface *api, data_accessor_t accessor):
        api_proxy(api), data_accessor(accessor)
        {}

    /// @brief Resyncs current cache with the API by timer. Obtains valid saving
    /// statuses for all the previously requested and delayed item ids. Updates
    /// main cache `c` immediately.
    /// NOTE: the class has an access to the data container through `data_accessor_t` member,
    /// but in the `resync` particular case `json_cache` creates a temp cache object, until
    /// resync is finished succesfully, which is not accessible over getter. So, it is
    /// passed here directly as an argument
    bool resync(statuses_container_t& c);

    /// @brief Sets `status` saving flag to all the given `ids` items in cache
    void update_saved_items(const item_ids_t &ids, bool status);

    /// @brief Returns items `item_id` saving status if known, otherwise `false`
    /// and add `item_id` to the queue for requesting
    bool is_item_saved(const item_id_t &item_id, bool force_sync);
protected:
    /// @brief Helps to get an access to the needed nested container, which is part of
    /// the main on `c`
    virtual auto get_container(collection_base_t::data_t& c) -> statuses_container_t& = 0;

    /// @brief Implements a specific checking API request for the item types the class holds
    virtual auto check_saved_items(api_interface *api, const item_ids_t &ids) -> std::deque<bool> = 0;
private:
    api_interface *api_proxy;
    data_accessor_t data_accessor;
    item_ids_t ids_to_process;
    std::mutex ids_access_guard;
};

/// @brief Class specialisation for caching tracks saving statuses
class tracks_items_cache_t: public saved_items_cache_t
{
public:
    using saved_items_cache_t::saved_items_cache_t;
protected:
    auto get_container(collection_base_t::data_t &data) -> statuses_container_t& override;
    auto check_saved_items(api_interface *api, const item_ids_t &ids) -> std::deque<bool> override;
};

/// @brief Class specialisation for caching albums saving statuses
class albums_items_cache_t: public saved_items_cache_t
{
public:
    using saved_items_cache_t::saved_items_cache_t;
protected:
    auto get_container(collection_base_t::data_t &data) -> statuses_container_t& override;
    auto check_saved_items(api_interface *api, const item_ids_t &ids) -> std::deque<bool> override;
};


/// @brief A class-container for providing an access to the API items and user's own collection.
/// Caches the data, performs delayed request to reduce a workload to the API and sends events
/// on readiness.
class collection: public collection_base_t
{
public:
    collection(api_interface *api);
    ~collection();

    /// @brief Checks the given track `id` saving status. Returns immediately if is cached,
    /// otherwise returns `false` and puts to the queue for requesting.
    /// @param force_sync forces method to check the status synchroniously
    /// 
    /// https://developer.spotify.com/documentation/web-api/reference/check-users-saved-tracks 
    bool is_track_saved(const item_id_t &id, bool force_sync = false);

    /// @brief https://developer.spotify.com/documentation/web-api/reference/save-tracks-user
    bool save_tracks(const item_ids_t &ids);

    /// @brief https://developer.spotify.com/documentation/web-api/reference/remove-tracks-user
    bool remove_saved_tracks(const item_ids_t &ids);

    /// @brief Checks the given album `id` saving status. Returns immediately if is cached,
    /// otherwise returns `false` and puts to the queue for requesting.
    /// @param force_sync forces method to check the status synchroniously
    /// 
    /// @brief https://developer.spotify.com/documentation/web-api/reference/check-users-saved-albums
    bool is_album_saved(const item_id_t &id, bool force_sync = false);

    /// @brief https://developer.spotify.com/documentation/web-api/reference/save-albums-user 
    bool save_albums(const item_ids_t &ids);

    /// @brief https://developer.spotify.com/documentation/web-api/reference/remove-albums-user
    bool remove_saved_albums(const item_ids_t &ids);
protected:
    // json_cache's interface
    bool is_active() const override;
    bool request_data(data_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
private:
    api_interface *api_proxy;

    tracks_items_cache_t tracks;
    albums_items_cache_t albums;
};

struct collection_observer: public BaseObserverProtocol
{
    /// @brief The even is being thrown when the given tracks' `ids` saving statuses
    /// have been changed
    virtual void on_saved_tracks_changed(const item_ids_t &ids) {}

    /// @brief The even is being thrown when the given albums' `ids` saving statuses
    /// have been changed
    virtual void on_saved_albums_changed(const item_ids_t &ids) {}
};

} // namespace spotify
} // namespace spotifar

#endif // collection_HPP_92081AAB_EE4E_40B2_814E_83B712132465