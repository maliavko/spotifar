#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "abstract.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "auth.hpp"
#include "history.hpp"

namespace spotifar { namespace spotify {

class api:
    public api_abstract,
    public auth_observer
{
public:
    api();
    ~api();

    bool start();
    void shutdown();
    void tick();
    void clear_http_cache();

    bool is_authenticated() const { return auth->is_authenticated(); }

    void set_frequent_syncs(bool is_on) { is_frequent_syncs_flag = is_on; };
    bool is_frequent_syncs() const { return is_frequent_syncs_flag; }
    
    // library api interface
    auto get_available_devices() -> const devices_t& { return devices->get(); }
    auto get_playback_state() -> const playback_state& { return playback->get(); }
    auto get_artist(const string &artist_id) -> artist;
    auto get_artist_top_tracks(const string &artist_id) -> tracks_t;
    auto get_album(const string &album_id) -> album;
    auto get_album_tracks(const string &album_id) -> simplified_tracks_t;
    auto get_playlist(const string &playlist_id) -> playlist;
    auto get_playlist_tracks(const string &playlist_id) -> playlist_tracks_t;
    auto check_saved_track(const string &track_id) -> bool;
    auto check_saved_tracks(const std::vector<string> &ids) -> std::vector<bool>;
    auto save_tracks(const std::vector<string> &ids) -> bool;
    auto remove_saved_tracks(const std::vector<string> &ids) -> bool;
    auto get_playing_queue() -> playing_queue;

    // playback api interface
    void start_playback(const string &context_uri, const string &track_uri = "",
                        int position_ms = 0, const string &device_id = "");
    void start_playback(const std::vector<string> &uris, const string &device_id = "");
    void start_playback(const simplified_album &album, const simplified_track &track);
    void start_playback(const simplified_playlist &playlist, const simplified_track &track);
    void resume_playback(const string &device_id = "");
    void toggle_playback(const string &device_id = "");
    void pause_playback(const string &device_id = "");
    void skip_to_next(const string &device_id = "");
    void skip_to_previous(const string &device_id = "");
    void seek_to_position(int position_ms, const string &device_id = "");
    void toggle_shuffle(bool is_on, const string &device_id = "");
    void toggle_shuffle_plus(bool is_on);
    void set_repeat_state(const string &mode, const string &device_id = "");
    void set_playback_volume(int volume_percent, const string &device_id = "");
    void transfer_playback(const string &device_id, bool start_playing = false);
    
    // the main interface for raw http requests
    virtual httplib::Result get(const string &request_url, utils::clock_t::duration cache_for = {});
    virtual httplib::Result put(const string &request_url, const json &body = {});
    virtual httplib::Result del(const string &request_url, const json &body = {});

protected:
    void start_playback(const json &body, const string &device_id);
    
    void on_auth_status_changed(const auth &auth);

private:
    BS::thread_pool pool;
    httplib::Client client;
    bool is_frequent_syncs_flag = false;
    http_cache api_responses_cache;

    // caches
    std::unique_ptr<playback_cache> playback;
    std::unique_ptr<devices_cache> devices;
    std::unique_ptr<auth_cache> auth;
    std::unique_ptr<PlayedHistory> history;

    std::vector<cached_data_abstract*> caches;
};

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66