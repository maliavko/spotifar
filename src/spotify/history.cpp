#include "history.hpp"

namespace spotifar { namespace spotify {

namespace log = utils::log;
using utils::far3::synchro_tasks::dispatch_event;

play_history::play_history(api_abstract *api):
    json_cache<history_items_t>(L"play_history"),
    api_proxy(api)
    {};

bool play_history::is_active() const
{
    return api_proxy->is_authenticated();
}

clock_t::duration play_history::get_sync_interval() const
{
    return utils::events::has_observers<play_history_observer>() ? 5s : 2min;
}

bool play_history::request_data(history_items_t &data)
{
    // 10 seconds to cover some network gap
    auto last_sync_time = duration_cast<std::chrono::milliseconds>(
        get_last_sync_time().time_since_epoch()).count() - 10;

    data = get();

    // we request only the new items after the last request timestamp, then we take
    // the old items list and extend it from the front
    const auto &new_entries = api_proxy->get_recently_played(last_sync_time);
    data.insert(data.begin(), new_entries.begin(), new_entries.end());

    dispatch_event(&play_history_observer::on_items_updated, new_entries);

    return true;
}

} // namespace spotify
} // namespace spotifar