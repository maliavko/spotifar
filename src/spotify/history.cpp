#include "history.hpp"

namespace spotifar
{
    namespace spotify
    {
        PlayedHistory::PlayedHistory(api_abstract *api):
            json_cache(L"PlayedHistory"),
            api(api)
            {};

        clock_t::duration PlayedHistory::get_sync_interval() const
        {
            return 5min;
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

            string request_url = httplib::append_query_params("/v1/me/player/recently-played", params);

            auto res = api->get(httplib::append_query_params(
                "/v1/me/player/recently-played", params));
            auto history = json::parse(res->body).at("items").get<HistoryList>();
            data.insert(data.begin(), history.begin(), history.end());
            return true;
        }
        

        void from_json(const json &j, HistoryItem &p)
        {
            j.at("played_at").get_to(p.played_at);
            j.at("track").get_to(p.track);
            
            if (j.contains("context") && !j.at("context").is_null())
                j.at("context").get_to(p.context);
        }

        void to_json(json &j, const HistoryItem &p)
        {
            j = json{
                {"played_at", p.played_at},
                {"context", p.context},
                {"track", p.track},
            };
        }
    }
}