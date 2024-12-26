#include "history.hpp"

namespace spotifar
{
    namespace spotify
    {
        PlayedHistory::PlayedHistory(httplib::Client *endpoint):
            CachedValue(endpoint, L"PlayedHistory")
            {};

        std::chrono::milliseconds PlayedHistory::get_sync_interval() const
        {
            return std::chrono::milliseconds(15 * 1000);
        }

        bool PlayedHistory::request_data(HistoryList &data)
        {
            auto last_sync_time = duration_cast<std::chrono::milliseconds>(
                get_last_sync_time().time_since_epoch()).count();
            
            // TODO: error handling
            httplib::Params params{
                { "limit", "50" },
                { "after", std::to_string(last_sync_time) },
            };

            std::string request_url = httplib::append_query_params("/v1/me/player/recently-played", params);
            auto r = endpoint->Get(request_url);

            auto history = json::parse(r->body).at("items").get<HistoryList>();
            data.insert(data.begin(), history.begin(), history.end());
            return true;
        }
    }
}