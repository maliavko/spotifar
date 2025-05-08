#ifndef RELEASES_HPP_DFCA9F47_33C0_4CF0_8C6B_11103CA6E2FB
#define RELEASES_HPP_DFCA9F47_33C0_4CF0_8C6B_11103CA6E2FB
#pragma once

#include "stdafx.h"
#include "common.hpp"
#include "items.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

/// @brief A class-holder for the recent releases business logic: syncs, caches
/// and provides an access to the list of the recently released albums of the
/// followed artists.
///
/// The algorythms is the following: after a successful authorization the class
/// fetches the list of the followed artists, and after submits the separate request-task
/// for fetching each artist's albums into the thread-pool. There is a delay between each
/// request depending on the activity/intensity of the plugin requests. There is no delay
/// between requests, if the request's results was cached previously. If the process
/// finishes successfully eventually, the list is cached for 24 hours.
class recent_releases: public json_cache<recent_releases_t>
{
public:
    inline static const auto release_age = std::chrono::weeks{2};
public:
    recent_releases(api_interface *api);
    ~recent_releases();
protected:
    // json_cache's interface
    bool is_active() const override;
    bool request_data(data_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
    void on_data_synced(const data_t &data, const data_t &prev_data) override;
private:
    api_interface *api_proxy;
    BS::thread_pool pool;
    
    std::mutex data_access;
    data_t interim_data{}; // a container, accumulates the interimg requested data

    bool is_in_sync = false; // ongoing syncing flag
    followed_artists_ptr artists;
};

struct recent_releases_observer: public BaseObserverProtocol
{
    /// @brief Sends the event of some newely found recent albums of the followed artists
    virtual void on_recent_releases_found(const recent_releases_t releases) {}
};

} // namespace spotify
} // namespace spotifar

#endif // RELEASES_HPP_DFCA9F47_33C0_4CF0_8C6B_11103CA6E2FB