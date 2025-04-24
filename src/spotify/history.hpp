#ifndef CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#define CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#pragma once

#include "common.hpp"
#include "items.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

using recently_played_tracks_t = sync_collection<history_item_t>;
using recently_played_tracks_ptr = std::shared_ptr<recently_played_tracks_t>;

class play_history: public json_cache<history_items_t>
{
public:
    play_history(api_interface *api);
    ~play_history() { api_proxy = nullptr; }
protected:
    /// @brief https://developer.spotify.com/documentation/web-api/reference/get-recently-played
    auto get_recently_played(std::int64_t after) -> recently_played_tracks_ptr;

    // `json_cache` class overloads
    bool is_active() const override;
    bool request_data(history_items_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
    void on_data_synced(const history_items_t &data, const history_items_t &prev_data) override;
private:
    api_interface *api_proxy;
};

struct play_history_observer: public BaseObserverProtocol
{
    virtual void on_items_changed() {}
};

} // namespace spotify
} // namespace spotifar

#endif //CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72