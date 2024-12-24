#include "playback.hpp"
#include "abstract/observers.hpp"
#include "ObserverManager.h"

namespace spotifar
{
    namespace spotify
    {
        std::chrono::seconds PlaybackCache::get_sync_interval() const
        {
            return std::chrono::seconds(0);
        }
        
        void PlaybackCache::on_data_synced(PlaybackState &data)
        {
            ObserverManager::notify(&PlaybackObserver::on_playback_updated, data);
        }

        bool PlaybackCache::request_data(PlaybackState &data)
        {
            PlaybackState state;  // empty playback by default

            auto r = endpoint->Get("/v1/me/player");
            if (r->status == httplib::OK_200)
                state = json::parse(r->body).get<PlaybackState>();

            data = state;

            return true;
        }
    }
}