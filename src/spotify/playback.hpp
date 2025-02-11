#ifndef PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#define PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7
#pragma once

#include "devices.hpp"
#include "cache.hpp"
#include "items.hpp" // TODO: remove dependency on track

namespace spotifar { namespace spotify {

struct actions
{
    bool interrupting_playback = false;
    bool pausing = false;
    bool resuming = false;
    bool seeking = false;
    bool skipping_next = false;
    bool skipping_prev = false;
    bool toggling_repeat_context = false;
    bool toggling_repeat_track = false;
    bool toggling_shuffle = false;
    bool trasferring_playback = false;

    friend bool operator==(const actions &lhs, const actions &rhs);
    friend void from_json(const json &j, actions &p);
    friend void to_json(json &j, const actions &p);
};

struct context
{
    inline static const string
        album = "album",
        playlist = "playlist",
        artist = "artist",
        show = "show",
        collection = "collection";

    string type;
    string uri;
    string href;

    bool is_empty() const { return type == ""; }
    bool is_artist() const { return type == artist; }
    bool is_album() const { return type == album; }
    bool is_playlist() const { return type == playlist; }
    bool is_collection() const { return type == collection; }
    string get_item_id() const;

    friend bool operator==(const context &lhs, const context &rhs);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(context, type, uri, href);
};

// https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
struct playback_state
{
    inline static const string
        repeat_off = "off",
        repeat_track = "track",
        repeat_context = "context";

    spotify::actions actions;
    spotify::device device;
    string repeat_state = repeat_off;
    bool shuffle_state = false;
    int progress_ms = 0;
    int progress = 0;
    bool is_playing = false;
    track item;
    context context;

    inline bool is_empty() const { return item.id == ""; }
    friend void from_json(const json &j, playback_state &p);
    friend void to_json(json &j, const playback_state &p);
};

class playback_cache: public json_cache<playback_state>
{
public:
    playback_cache(api_abstract *api);
    virtual ~playback_cache() { api = nullptr; }
    virtual bool is_active() const;

    /// @param tracks_uris list of spotify tracks' URIs
    void activate_super_shuffle(const std::vector<string> &tracks_uris);
protected:
    virtual void on_data_synced(const playback_state &data, const playback_state &prev_data);
    virtual bool request_data(playback_state &data);
    virtual clock_t::duration get_sync_interval() const;

private:
    api_abstract *api;
};

struct playback_observer: public BaseObserverProtocol
{
    /// @brief A track has changed
    /// @param track a new track, which jsut started playing
    virtual void on_track_changed(const track &track) {};

    /// @brief A track's progress has changed
    /// @param duration a total track duration in seconds
    /// @param progress a current playing position in seconds
    virtual void on_track_progress_changed(int duration, int progress) {};

    /// @brief A volume has changed
    virtual void on_volume_changed(int volume) {};

    /// @brief A shuffle state has changed
    virtual void on_shuffle_state_changed(bool shuffle_state) {};

    /// @brief A repeat state has changed
    virtual void on_repeat_state_changed(const string &repeat_state) {};

    /// @brief A playback state has changed: us playing or not
    virtual void on_state_changed(bool is_playing) {};

    /// @brief A playing context has changed
    virtual void on_context_changed(const context &ctx) {};

    /// @brief Playing permissions have changed
    virtual void on_permissions_changed(const spotify::actions &actions) {};
};

} // namespace spotify
} // namespace spotifar

#endif // PLAYBACK_HPP_1E84D5C4_F3BB_4BCA_8719_1995E4AF0ED7