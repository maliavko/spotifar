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
#include "library.hpp"

namespace spotifar { namespace spotify {

class api: public api_abstract
{
public:
    api();
    virtual ~api();

    bool start();
    void shutdown();
    void tick();

    template<class T> void start_listening(T *o);
    template<class T> void stop_listening(T *o);

    SimplifiedTracksT get_album_tracks(const string &album_id);
    PlaylistTracksT get_playlist_tracks(const string &playlist_id);
    tracks_list_t get_artist_top_tracks(const string &artist_id);
    AlbumsCollection get_albums(const string &artist_id);
    PlaylistsCollection get_playlists();
    
    inline const devices_list_t& get_available_devices() { return devices->get(); }
    inline const playback_state& get_playback_state() { return playback->get(); }
    inline virtual bool is_authenticated() const { return auth->is_authenticated(); }
    inline virtual size_t get_playback_observers_count() const { return playback_observers; }

    virtual httplib::Result get(const string &request_url, utils::clock_t::duration cache_for = {});
    LibraryCache& get_library() { return *library; }

    void start_playback(const string &context_uri, const string &track_uri = "",
                        int position_ms = 0, const string &device_id = "");
    void start_playback(const std::vector<string> &uris, const string &device_id = "");
    void start_playback(const simplified_album &album, const SimplifiedTrack &track);
    void start_playback(const SimplifiedPlaylist &playlist, const SimplifiedTrack &track);
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

protected:
    void start_playback(const json &body, const string &device_id);
    virtual void set_bearer_token_auth(const string &token);

private:
    BS::thread_pool pool;
    httplib::Client client;
    size_t playback_observers = 0;

    json responses_cache; // TODO: not sure it is a final solution

    // caches
    std::unique_ptr<playback_cache> playback;
    std::unique_ptr<devices_cache> devices;
    std::unique_ptr<auth_cache> auth;
    std::unique_ptr<PlayedHistory> history;
    std::unique_ptr<LibraryCache> library;

    std::vector<cached_data_abstract*> caches;
};

template<class T>
void api::start_listening(T *o)
{
    ObserverManager::subscribe<T>(o);
    // if the observer is a playback observer, we let API know to increase the
    // frequency of playback state updates
    if (std::is_same<T, playback_observer>::value)
        ++playback_observers;
}

template<class T>
void api::stop_listening(T *o)
{
    ObserverManager::unsubscribe<T>(o);
    if (std::is_same<T, playback_observer>::value)
        --playback_observers;
}

} // namespace spotify
} // namespace spotifar

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66