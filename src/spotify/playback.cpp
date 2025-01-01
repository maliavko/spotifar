#include "stdafx.h"
#include "playback.hpp"
#include "abstract/observers.hpp"
#include "api.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;

        utils::ms PlaybackCache::get_sync_interval() const
        {
            return utils::ms(950ms);
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
            auto api_ptr = dynamic_cast<Api*>(api);
            auto res = api_ptr->client.Get("/v1/me/player");
            if (res->status == httplib::OK_200)
            {
                auto j = json::parse(res->body);
                apply_patches(j).get_to(data);
                return true;
            }
            else if (res->status == httplib::NoContent_204)
            {
                // we make sure that the stored last played data has "is_playing = false",
                // not to confuse anybody with the UI status
                data = get_data();
                if (!data.is_empty())
                    data.is_playing = false;
                return true;
            }

            return false;
        }
    }
}