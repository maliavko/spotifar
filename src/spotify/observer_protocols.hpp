#ifndef OBSERVER_PROTOCOL_HPP_4E1D7168_14D2_4C8B_A374_5C1DA9CDB8A7
#define OBSERVER_PROTOCOL_HPP_4E1D7168_14D2_4C8B_A374_5C1DA9CDB8A7
#pragma once

#include "stdafx.h"
#include "items.hpp"

namespace spotifar { namespace spotify
{
    
struct api_requests_observer: public BaseObserverProtocol
{
    /// @brief The event is called, when some multi-page requester is about
    /// to execute a remote request
    /// @param url a request's url to identify requests from each other
    virtual void on_request_started(const string &url) {}

    /// @brief The event is called, when some multi-page requester is about
    /// to finish a remote request execution
    /// @param url a request's url to identify requests from each other
    virtual void on_request_finished(const string &url) {}

    /// @brief Some multi-page requester has received an intermediate result
    /// @param url a requester url
    /// @param progress an amount of data entries accumulated so far
    /// @param total an amount of total entries to receive
    virtual void on_request_progress_changed(const string &url, size_t progress, size_t total) {}

    /// @brief The controlling playback command is failed: start_playback, skip_to_next and etc.
    virtual void on_playback_command_failed(const string &message) {}

    /// @brief The collection fetching was failed
    virtual void on_collection_fetching_failed(const string &message) {}
};


struct collection_observer: public BaseObserverProtocol
{
    /// @brief The even is fired when the given tracks `ids` saving statuses have been changed
    virtual void on_tracks_statuses_changed(const item_ids_t &) {}

    /// @brief The event is fired  when the given tracks statuses are received
    virtual void on_tracks_statuses_received(const item_ids_t &) {}

    /// @brief The even is fired when the given albums `ids` saving statuses have been changed
    virtual void on_albums_statuses_changed(const item_ids_t &) {}

    /// @brief The event is fired  when the given albums statuses are received
    virtual void on_albums_statuses_received(const item_ids_t &) {}

    /// @brief The even is fired when the given artists `ids` saving statuses have been changed
    virtual void on_artists_statuses_changed(const item_ids_t &) {}

    /// @brief The event is fired  when the given artists statuses are received
    virtual void on_artists_statuses_received(const item_ids_t &) {}
};


struct auth_observer: public BaseObserverProtocol
{
    /// @brief An auth status has been changed. For the first login `is_renewal` flag
    /// will be `false`
    virtual void on_auth_status_changed(const auth_t &auth, bool is_renewal) {}
};


struct devices_observer: public BaseObserverProtocol
{
    /// @brief A list of available devices has been changed
    virtual void on_devices_changed(const devices_t &devices) {}
};


struct playback_observer: public BaseObserverProtocol
{
    /// @brief A track has changed
    /// @param track a new track, which jsut started playing
    /// @param prev_track a previous track, which was playing before
    virtual void on_track_changed(const track_t &track, const spotify::track_t &prev_track) {}

    /// @brief A track's progress has changed
    /// @param duration a total track duration in seconds
    /// @param progress a current playing position in seconds
    virtual void on_track_progress_changed(int duration, int progress) {}

    /// @brief A volume has changed
    virtual void on_volume_changed(int volume) {}

    /// @brief A shuffle state has changed
    virtual void on_shuffle_state_changed(bool shuffle_state) {}

    /// @brief A repeat state has changed
    virtual void on_repeat_state_changed(const string &repeat_state) {}

    /// @brief A playback state has changed: us playing or not
    virtual void on_state_changed(bool is_playing) {}

    /// @brief A playing context has changed
    virtual void on_context_changed(const context_t &ctx) {}

    /// @brief Playing permissions have changed
    virtual void on_permissions_changed(const actions_t &actions) {}
};


struct play_history_observer: public BaseObserverProtocol
{
    virtual void on_history_changed() {}
};


struct releases_observer: public BaseObserverProtocol
{
    /// @brief The event is thrown when the recent releases seach procedure is finished
    virtual void on_releases_sync_finished(const recent_releases_t releases) {}
};

} // namespace spotify
} // namespace spotifar

#endif // OBSERVER_PROTOCOL_HPP_4E1D7168_14D2_4C8B_A374_5C1DA9CDB8A7