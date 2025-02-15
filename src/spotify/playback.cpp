#include "stdafx.h"
#include "playback.hpp"

namespace spotifar { namespace spotify {

namespace log = utils::log;
using utils::far3::synchro_tasks::dispatch_event;

playback_cache::playback_cache(api_abstract *api): json_cache(L"PlaybackState"), api(api) {}

bool playback_cache::is_active() const
{
    // the cache is actively synchronized only when the user is authenticated and there are
    // playback observers
    return api->is_authenticated() && api->is_playback_active();
}

clock_t::duration playback_cache::get_sync_interval() const
{
    // every second, minus some gap for smoother synching
    return 950ms;
}

void playback_cache::on_data_synced(const playback_state &data, const playback_state &prev_data)
{
    if (data.item != prev_data.item)
        dispatch_event(&playback_observer::on_track_changed, data.item);

    if (data.progress_ms != prev_data.progress_ms)
        dispatch_event(&playback_observer::on_track_progress_changed, data.item.duration, data.progress);

    if (data.device.volume_percent != prev_data.device.volume_percent)
        dispatch_event(&playback_observer::on_volume_changed, data.device.volume_percent);

    if (data.shuffle_state != prev_data.shuffle_state)
        dispatch_event(&playback_observer::on_shuffle_state_changed, data.shuffle_state);

    if (data.repeat_state != prev_data.repeat_state)
        dispatch_event(&playback_observer::on_repeat_state_changed, data.repeat_state);

    if (data.is_playing != prev_data.is_playing)
        dispatch_event(&playback_observer::on_state_changed, data.is_playing);

    if (data.context != prev_data.context)
        dispatch_event(&playback_observer::on_context_changed, data.context);

    if (data.actions != prev_data.actions)
        dispatch_event(&playback_observer::on_permissions_changed, data.actions);

    // TODO: send changes in permissions
}

bool playback_cache::request_data(playback_state &data)
{
    auto res = api->get("/v1/me/player");
    if (res->status == httplib::OK_200)
    {
        json::parse(res->body).get_to(data);
        return true;
    }
    else if (res->status == httplib::NoContent_204)
    {
        // we make sure that the stored last played data has "is_playing = false",
        // not to confuse anybody with the UI status
        data = get();
        if (!data.is_empty())
        {
            data.is_playing = false;
        }
        return true;
    }

    return false;
}

void playback_cache::activate_super_shuffle(const std::vector<string> &tracks_uris)
{
    log::api->debug("activate_super_shuffle, traks number: {}", tracks_uris.size());

    auto uris = tracks_uris;
    std::random_device rd{}; 
    std::default_random_engine rng{ rd() };
    std::ranges::shuffle(uris, rng);

    // TODO: finish up, avoid includ api.hpp
    // uris.resize(100); // soft-cap for now, just first 100 tracks
    // api* a = dynamic_cast<api*>(api);
    // a->start_playback(uris);
}

void from_json(const json &j, playback_state &p)
{
    j.at("device").get_to(p.device);
    j.at("repeat_state").get_to(p.repeat_state);
    j.at("shuffle_state").get_to(p.shuffle_state);
    j.at("is_playing").get_to(p.is_playing);
    j.at("actions").get_to(p.actions);

    p.progress_ms = j.value("progress_ms", 0);
    p.progress = p.progress_ms / 1000;

    if (j.contains("context") && !j.at("context").is_null())
        j.at("context").get_to(p.context);

    if (j.contains("item") && !j.at("item").is_null())
        j.at("item").get_to(p.item);
}

void to_json(json &j, const playback_state &p)
{
    j = json{
        { "device", p.device },
        { "repeat_state", p.repeat_state },
        { "shuffle_state", p.shuffle_state },
        { "progress_ms", p.progress_ms },
        { "is_playing", p.is_playing },
        { "actions", p.actions },
        { "item", p.item },
        { "context", p.context },
    };
}

bool operator==(const actions &lhs, const actions &rhs)
{
    return (
        lhs.interrupting_playback == rhs.interrupting_playback &&
        lhs.pausing == rhs.pausing &&
        lhs.resuming == rhs.resuming &&
        lhs.seeking == rhs.seeking &&
        lhs.skipping_next == rhs.skipping_next &&
        lhs.skipping_prev == rhs.skipping_prev &&
        lhs.toggling_repeat_context == rhs.toggling_repeat_context &&
        lhs.toggling_repeat_track == rhs.toggling_repeat_track &&
        lhs.toggling_shuffle == rhs.toggling_shuffle &&
        lhs.trasferring_playback == rhs.trasferring_playback
    );
}

void from_json(const json &j, actions &p)
{
    if (j.is_null())
        return;
    
    p.interrupting_playback = j.value("interrupting_playback", false);
    p.pausing = j.value("pausing", false);
    p.resuming = j.value("resuming", false);
    p.seeking = j.value("seeking", false);
    p.skipping_next = j.value("skipping_next", false);
    p.skipping_prev = j.value("skipping_prev", false);
    p.toggling_repeat_context = j.value("toggling_repeat_context", false);
    p.toggling_repeat_track = j.value("toggling_repeat_track", false);
    p.toggling_shuffle = j.value("toggling_shuffle", false);
    p.trasferring_playback = j.value("trasferring_playback", false);
}

void to_json(json &j, const actions &p)
{
    // TODO: unfinished
    j = json{};
}

string context::get_item_id() const
{
    static auto r = std::regex("(\\w+):(\\w+):(\\w+)");	
    std::smatch match;
    if (std::regex_search(uri, match, r))
        return match[3];
    return "";
}

bool operator==(const context &lhs, const context &rhs)
{
    return lhs.href == rhs.href;
}

} // namespace spotify
} // namespace spotifar