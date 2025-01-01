#include "history.hpp"
#include "api.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;

        PlayedHistory::PlayedHistory(IApi *api):
            CachedValue(L"PlayedHistory"),
            api(api)
            {};

        utils::ms PlayedHistory::get_sync_interval() const
        {
            return utils::ms(5min);
        }

        bool PlayedHistory::request_data(HistoryList &data)
        {
            auto last_sync_time = duration_cast<utils::ms>(
                get_last_sync_time().time_since_epoch()).count();
            
            // TODO: error handling
            httplib::Params params{
                { "limit", "50" },
                { "after", std::to_string(last_sync_time) },
            };

            std::string request_url = httplib::append_query_params("/v1/me/player/recently-played", params);

            auto api_ptr = dynamic_cast<Api*>(api);
            auto res = api_ptr->client.Get(httplib::append_query_params(
                "/v1/me/player/recently-played", params));
            auto history = json::parse(res->body).at("items").get<HistoryList>();
            data.insert(data.begin(), history.begin(), history.end());
            return true;
        }
    }
}