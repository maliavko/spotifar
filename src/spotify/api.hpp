#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "httplib.h"
#include "items.hpp"
#include "abstract/cached_value.hpp"
#include "abstract/observers.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "auth.hpp"
#include "history.hpp"

namespace spotifar
{
    namespace spotify
    {
        using std::string;
        using std::wstring;

        class Api
        {
        public:
            Api();
            virtual ~Api();

            bool init();
            void shutdown();

            template<class T> void start_listening(T *o);
            template<class T> void stop_listening(T *o);

            ArtistsCollection get_artists();
            AlbumsCollection get_albums(const string &artist_id);
            PlaylistsCollection get_playlists();
            std::map<string, SimplifiedTrack> get_tracks(const string &album_id);

            // NOTE: no args means "resume"
            void start_playback(const string &context_uri = "", const string &track_uri = "",
                                int position_ms = 0, const string &device_id = "");
            void start_playback(const SimplifiedAlbum &album, const SimplifiedTrack &track);
            void start_playback(const SimplifiedPlaylist &playlist, const SimplifiedTrack &track);
            void pause_playback(const string &device_id = "");
            void skip_to_next();
            void skip_to_previous();
            void seek_to_position(int position_ms, const string &device_id = "");
            void toggle_shuffle(bool is_on, const string &device_id = "");
            void set_repeat_state(const std::string &mode, const string &device_id = "");
            void set_playback_volume(int volume_percent);
            bool transfer_playback(const string &device_id, bool start_playing = false);

            inline const DevicesList& get_available_devices() { return devices->get_data(); }
            inline const PlaybackState& get_playback_state() { return playback->get_data(); }

        protected:
            void launch_sync_worker();
            void shutdown_sync_worker();

            inline PlaybackCache& get_playback_cache() { return *playback; }
            inline DevicesCache& get_devices_cache() { return *devices; }

        private:
            httplib::Client endpoint;
            size_t playback_observers;

            std::shared_ptr<spdlog::logger> logger;
            std::mutex sync_worker_mutex;
            bool is_worker_listening;

            // caches
            std::unique_ptr<PlaybackCache> playback;
            std::unique_ptr<DevicesCache> devices;
            std::unique_ptr<AuthCache> auth;
            std::unique_ptr<PlayedHistory> history;

            std::vector<ICachedValue*> caches;
        };
        
        template<class T>
        void Api::start_listening(T *o)
        {
            ObserverManager::subscribe<T>(o);

            if (std::is_same<T, PlaybackObserver>::value)
            {
                bool is_playback_active = ++playback_observers > 0;
                playback->set_enabled(is_playback_active);
                devices->set_enabled(is_playback_active);
            }
        }

        template<class T>
        void Api::stop_listening(T *o)
        {
            if (std::is_same<T, PlaybackObserver>::value)
            {
                bool is_playback_active = --playback_observers > 0;
                playback->set_enabled(is_playback_active);
                devices->set_enabled(is_playback_active);
            }
            ObserverManager::unsubscribe<T>(o);
        }
    }
}

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66