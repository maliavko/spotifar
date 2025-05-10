#include "releases.hpp"
#include "playback.hpp"

namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;

recent_releases::recent_releases(api_interface *api):
    json_cache<data_t>(L""), api_proxy(api), pool(1)
{
    artists = api_proxy->get_followed_artists();
}

recent_releases::~recent_releases()
{
    api_proxy = nullptr;
    pool.purge();
}

bool recent_releases::is_active() const
{
    return api_proxy->is_authenticated();
}

bool recent_releases::request_data(data_t &data)
{
    // NOTE: a normal behaviour of this method is to return `false` in case of error and `true`,
    // if resync went well and data is received successfully. As we have here a long sync,
    // which takes place over a bunch of delayed threaded requests, the responce is considered
    // successful only when the last request is finished with no errors.

    if (is_in_sync)
    {
        // if there are no queued or ongoing tasks, we finish up the synching process:
        // chaging the `is_in_sync` flag
        if (pool.get_tasks_total() == 0)
        {
            data = interim_data;
            is_in_sync = false;
        }
        
        // if we are not in sync anymore - return `true`, the base class will store
        // the caches and fire the appropriate observing events if needed
        return !is_in_sync;
    }

    interim_data.clear();

    if (artists->fetch(false, false))
    {
        auto time_treshold = utils::clock_t::now() - release_age;
        
        pool.detach_sequence<size_t>(0, artists->size(),
            [this, time_treshold](const std::size_t idx)
            {
                const auto &artist = (*artists)[idx];

                auto albums = api_proxy->get_artist_albums(artist.id);
                bool is_cached = albums->is_cached();
                if (albums->fetch(false, false))
                {
                    std::lock_guard lock(data_access);

                    for (const auto &album: *albums)
                        if (album.get_release_date() > time_treshold)
                        {
                            log::api->info("A new release was found for the artist {} [{}]: {} [{}], {}",
                                utils::to_string(artist.name), artist.id, utils::to_string(album.name),
                                album.id, album.get_release_year());
                            
                            interim_data.push_back(album);
                        }
                }

                // we put the thread to sleep to avoid spamming spotify API, in case
                // the result was obtained from the cache, there is no need to do that
                if (!is_cached)
                    std::this_thread::sleep_for(
                        utils::events::has_observers<playback_observer>() ? 15s : 5s);
            });
        
        is_in_sync = true;
    }

    return false;
}

clock_t::duration recent_releases::get_sync_interval() const
{
    return 24h;
}

void recent_releases::on_data_synced(const data_t &data, const data_t &prev_data)
{
    log::global->info("A recent releases cache is found, next update in {}",
        std::format("{:%T}", get_expires_at() - clock_t::now()));

    const std::unordered_set<simplified_album_t> prev_releases(prev_data.begin(), prev_data.end());

    recent_releases_t result;
    for (const auto &album: data)
        if (!prev_releases.contains(album))
            result.push_back(album);

    dispatch_event(&releases_observer::on_releases_sync_finished, result);
}
    
} // namespace spotify
} // namespace spotifar