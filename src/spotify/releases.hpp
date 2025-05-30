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
    BS::light_thread_pool pool;

    // each request is slowed down to avoid API spamming by putting a thread into sleep;
    // to make it controllable and avoid blociking of application during closing e.g.,
    // the sleeping is done via conditional variable
    std::condition_variable cv;
    std::mutex cv_m;
    bool stop_flag = false;
    
    data_t interim_data{}; // a container, accumulates the interim requested data

    bool is_in_sync = false; // a syncing procedure is in action flag
    followed_artists_ptr artists; // collection of followed artists to go over
};

struct releases_observer: public BaseObserverProtocol
{
    /// @brief The event is thrown when the recent releases seach procedure is finished
    virtual void on_releases_sync_finished(const recent_releases_t releases) {}
};

} // namespace spotify
} // namespace spotifar

#endif // RELEASES_HPP_DFCA9F47_33C0_4CF0_8C6B_11103CA6E2FB