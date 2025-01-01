#ifndef CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#define CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "config.hpp"
#include "abstract/cached_value.hpp"

namespace spotifar
{
    namespace spotify
    {
        class PlayedHistory: public CachedValue<HistoryList>
        {
        public:
            PlayedHistory(IApi *api);
            virtual ~PlayedHistory() { api = nullptr; }
            // TODO: on_data_resync with the observer events

        protected:
            virtual utils::ms get_sync_interval() const;
            virtual bool request_data(HistoryList &data);

        private:
            std::shared_ptr<spdlog::logger> logger;
            IApi *api;
        };
    }
}

#endif //CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72