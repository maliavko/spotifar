#ifndef PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#define PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#pragma once

#include "cached_value.hpp"
#include "items.hpp"

namespace spotifar
{
    namespace spotify
    {
        class PlaybackCache: public CachedItem<PlaybackState>
        {
        public:
            PlaybackCache(IApi *api):
                CachedItem(L"PlaybackState", false),
                api(api)
                {}

            virtual ~PlaybackCache() { api = nullptr; }

        protected:
            virtual void on_data_synced(const PlaybackState &data, const PlaybackState &prev_data);
            virtual bool request_data(PlaybackState &data);
            virtual utils::ms get_sync_interval() const;

        private:
            std::shared_ptr<spdlog::logger> logger;
            IApi *api;
        };
    }
}

#endif // PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7