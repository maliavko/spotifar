#ifndef PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#define PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#pragma once

#include "abstract/cached_value.hpp"
#include "items.hpp"

namespace spotifar
{
    namespace spotify
    {
        class PlaybackCache: public CachedValue<PlaybackState>
        {
        public:
            PlaybackCache(httplib::Client *endpoint):
                CachedValue(endpoint, L"PlaybackState", false)
                {}

        protected:
            virtual bool request_data(PlaybackState &data);
            virtual void on_data_synced(PlaybackState &data);
            virtual std::chrono::seconds get_sync_interval() const;

        private:
            std::shared_ptr<spdlog::logger> logger;
        };
    }
}

#endif // PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7