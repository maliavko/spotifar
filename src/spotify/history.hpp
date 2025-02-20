#ifndef CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#define CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72
#pragma once

#include "cache.hpp"
#include "items.hpp"

namespace spotifar
{
    namespace spotify
    {
        struct HistoryItem
        {
            track track;
            spotify::context context;
            string played_at;
            
            friend void from_json(const json &j, HistoryItem &p);
            friend void to_json(json &j, const HistoryItem &p);
        };
        
        typedef std::vector<HistoryItem> HistoryList;

        class PlayedHistory: public json_cache<HistoryList>
        {
        public:
            PlayedHistory(api_abstract *api);
            virtual ~PlayedHistory() { api = nullptr; }
            // TODO: on_data_resync with the observer events

        protected:
            virtual clock_t::duration get_sync_interval() const;
            virtual bool request_data(HistoryList &data);

        private:
            api_abstract *api;
        };
    }
}

#endif //CACHE_HPP_6BE0F54C_3187_4D5F_AC83_CE8B78223B72