#include "library.hpp"

namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;

library::library(api_interface *api): api_proxy(api), pool(1)
{
}

library::~library()
{
    stop_flag = true;
    cv.notify_all();
    
    api_proxy = nullptr;
    pool.purge();
}


void library::request_saved_status(const item_ids_t &ids)
{
    pool.detach_task(
        [this, ids]
        {
            std::unique_lock<std::mutex> thread_lock(cv_m);

            //bool is_cached = api_proxy->is_request_cached("/v1/me/tracks/contains", ids, 50);

            auto requester = several_items_requester<bool, -1, utils::clock_t::duration, std::deque<bool>>("/v1/me/tracks/contains", ids, 50);
            bool is_cached = requester.is_cached();

            auto result = api_proxy->check_saved_tracks(ids);
            if (result.size() == ids.size())
            {
                saved_status_t saved_status;
                for (size_t i = 0; i < ids.size(); ++i)
                    saved_status.insert({ ids[i], result[i] });

                dispatch_event(&collection_observer::on_saved_tracks_status_received, saved_status);
            }
            else
            {
                log::api->error("Failed to get saved status for {} tracks", ids.size());
            }

            cv.wait_for(thread_lock, 0.5s, [this]{ return stop_flag; });
        });
}

/*bool library::request_data(data_t &data)
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
                std::unique_lock<std::mutex> thread_lock(cv_m);

                const auto &artist = (*artists)[idx];

                auto albums = api_proxy->get_artist_albums(artist.id);
                bool is_cached = albums->is_cached();

                log::api->debug("Processing new artist's releases '{}' [{}], {} left",
                    utils::to_string(artist.name), artist.id, pool.get_tasks_total());
                
                if (albums->fetch(false, false))
                {
                    for (const auto &album: *albums)
                        if (album.get_release_date() > time_treshold)
                        {
                            log::api->info("A new release was found for the artist '{}' [{}]: {} [{}]",
                                utils::to_string(artist.name), artist.id,
                                utils::to_string(album.name), album.id);
                            
                            interim_data.push_back(album);
                        }
                }

                // we put the thread to sleep to avoid spamming spotify API; in case
                // the result was obtained from the cache, there is no need to do that
                if (!is_cached)
                {
                    auto delay_time = utils::events::has_observers<playback_observer>() ? 30s : 10s;
                    cv.wait_for(thread_lock, delay_time, [this]{ return stop_flag; });
                }
            });
        
        is_in_sync = true;
    }

    return false;
}

void library::on_data_synced(const data_t &data, const data_t &prev_data)
{
    log::global->info("A recent releases cache is found, next update in {}",
        std::format("{:%T}", get_expires_at() - clock_t::now()));

    const std::unordered_set<simplified_album_t> prev_releases(prev_data.begin(), prev_data.end());

    recent_releases_t result;
    for (const auto &album: data)
        if (!prev_releases.contains(album))
            result.push_back(album);

    if (result.size() > 0)
        dispatch_event(&releases_observer::on_releases_sync_finished, result);
}*/
    
} // namespace spotify
} // namespace spotifar