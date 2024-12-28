#include "stdafx.h"
#include "playback.hpp"
#include "abstract/observers.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;

        std::chrono::milliseconds PlaybackCache::get_sync_interval() const
        {
            return std::chrono::milliseconds(950ms);
        }
    
        void PlaybackCache::on_data_synced(const PlaybackState &data, const PlaybackState &prev_data)
        {
            if (data.item != prev_data.item)
                ObserverManager::notify(&PlaybackObserver::on_track_changed, data.item);

            if (data.progress_ms != prev_data.progress_ms)
                ObserverManager::notify(&PlaybackObserver::on_track_progress_changed,
                                        data.item.duration, data.progress);

            if (data.device.volume_percent != prev_data.device.volume_percent)
                ObserverManager::notify(&PlaybackObserver::on_volume_changed,
                                        data.device.volume_percent);

            if (data.shuffle_state != prev_data.shuffle_state)
                ObserverManager::notify(&PlaybackObserver::on_shuffle_state_changed,
                                        data.shuffle_state);

            if (data.repeat_state != prev_data.repeat_state)
                ObserverManager::notify(&PlaybackObserver::on_repeat_state_changed,
                                        data.repeat_state);

            if (data.is_playing != prev_data.is_playing)
                ObserverManager::notify(&PlaybackObserver::on_state_changed,
                                        data.is_playing);

            if (data.context != prev_data.context)
                ObserverManager::notify(&PlaybackObserver::on_context_changed, data.context);

            if (data.actions != prev_data.actions)
                ObserverManager::notify(&PlaybackObserver::on_permissions_changed, data.actions);

            // TODO: send changes in permissions
        }

        bool PlaybackCache::request_data(PlaybackState &data)
        {
            auto r = endpoint->Get("/v1/me/player");
            if (r->status == httplib::OK_200)
            {
                json::parse(r->body).get_to(data);
                return true;
            }
            else if (r->status == httplib::NoContent_204)
            {
                // the playback data is empty here, we skip it to continue showing
                // the last played real data in the player UI
                data = get_data();
                if (!data.is_empty())
                    data.is_playing = false;
                return true;
            }

            return false;
        }
        
        void PlaybackCache::on_data_patched(PlaybackState &data)
        {
            // patching will mark cache is valid, so it will prevent it from invalidating
            // for additional time, which is required for smooth track bar ticking.
            // To improve this case we update timings in cache as it were ticking themselves
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                clock::now() - get_last_sync_time()).count();
            
            data.progress_ms += (int)delta;
            data.progress = data.progress_ms / 1000;
        }
    }
}