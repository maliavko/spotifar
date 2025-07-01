#include "releases.hpp"
#include "requesters.hpp"
#include "observer_protocols.hpp"

namespace spotifar { namespace spotify {

using utils::far3::synchro_tasks::dispatch_event;

recent_releases::recent_releases(api_interface *api):
    json_cache<data_t>(L"recent_releases"), api_proxy(api), pool(1)
{
    artists = api_proxy->get_library()->get_followed_artists();

    utils::events::start_listening<collection_observer>(this);
}

recent_releases::~recent_releases()
{
    utils::events::stop_listening<collection_observer>(this);

    if (!stop_flag)
    {
        pool.purge();

        stop_flag = true;
        sleep_cv.notify_all();
    }
    
    api_proxy = nullptr;
}

void recent_releases::invalidate()
{
    json_cache::invalidate(1s);
}

bool recent_releases::is_cache_running() const
{
    return is_in_sync;
}

size_t recent_releases::get_sync_tasks_left() const
{
    return pool.get_tasks_total();
}

const recent_releases_t& recent_releases::get_items(bool force_resync)
{
    resync(force_resync);
    return get();
}

const utils::clock_t::time_point recent_releases::get_next_sync_time() const
{
    return this->get_expires_at();
}

bool recent_releases::is_active() const
{
    if (auto auth = api_proxy->get_auth_cache())
        return auth->is_authenticated();
    return false;
}

clock_t::duration recent_releases::get_sync_interval() const
{
    // the finally created cache is saved for one day
    return 24h;
}

void recent_releases::queue_artists(const item_ids_t &ids)
{
    pool.detach_sequence<size_t>(0, ids.size(),
        [
            this, ids,
            time_treshold = utils::clock_t::now() - release_age
        ]
        (const std::size_t idx)
        {
            std::unique_lock<std::mutex> thread_lock(sleep_cv_guard);

            const auto artist_id = ids[idx];

            // to the point when the task getting to its execution, the artist
            // could have been removed from collection, so skipping it quickly
            if (!api_proxy->get_library()->is_artist_followed(artist_id)) return;

            auto albums = api_proxy->get_artist_albums(artist_id, { "album", "single", "appears_on", "compilation" });
            bool is_cached = albums->is_cached();
                            
            dispatch_event(&releases_observer::on_sync_progress_changed, pool.get_tasks_total());
            
            if (albums->fetch(false, false))
            {
                for (const auto &album: *albums)
                    if (album.get_release_date() > time_treshold)
                    {
                        log::api->info("A new release was found for the artist '{}' - {}",
                            artist_id, album.id);
                        
                        interim_data.insert(album);
                    }
            }

            // we put the thread to sleep to avoid spamming spotify API; in case
            // the result was obtained from the cache, there is no need to do that
            if (!is_cached)
            {
                auto is_playback_active = utils::events::has_observers<playback_observer>();

                // the delay value depends on the playback state, which creates a heavy workload to the API;
                // by default if the response was brand-new, we delay thread longer, than if the response
                // was with 304 NotModified reponse and the data was taken from local cache
                auto delay_time = is_playback_active ? 30s : 10s;
                
                if (!albums->is_modified())
                    delay_time = is_playback_active ? 5s : 2s;

                sleep_cv.wait_for(thread_lock, delay_time, [this]{ return stop_flag; });
            }
        });
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
            data.assign(interim_data.begin(), interim_data.end());
            interim_data.clear();

            is_in_sync = false;
        }
        
        // if we are not in sync anymore - return `true`, the base class will store
        // the caches and fire the appropriate observing events if needed
        return !is_in_sync;
    }

    if (artists->fetch(false, false))
    {
        is_in_sync = true;

        item_ids_t ids;
        std::transform(artists->cbegin(), artists->cend(), std::back_inserter(ids),
            [](const auto &t) { return t.id; });

        queue_artists(ids);
    }

    return false;
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

    if (result.size() > 0)
        dispatch_event(&releases_observer::on_releases_sync_finished, result);

    dispatch_event(&releases_observer::on_sync_progress_changed, 0);
}

void recent_releases::on_artists_statuses_changed(const item_ids_t &ids)
{
    if (is_in_sync)
    {
        // enqueueing all the changed items, if the sync process is still ongoing
        item_ids_t added_ids;
        for (const auto &id: ids)
            if (api_proxy->get_library()->is_artist_followed(id))
                added_ids.push_back(id);
        
        // NOTE: we cannot remove the tasks enqueued for the removed artists, so the task itself
        // checks this particular case and skips if needed

        queue_artists(ids);
    }
    else
    {
        // ... or invalidating current cache, and forcing it to get resync, though
        // the cached items should get resynced very quickly
        json_cache::invalidate(10s);
    }
}

} // namespace spotify
} // namespace spotifar