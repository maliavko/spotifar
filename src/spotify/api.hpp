#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "httplib.h"
#include "items.hpp"
#include "abstract/cached_value.hpp"
#include "abstract/observers.hpp"

namespace spotifar
{
    namespace spotify
    {
        using std::string;
        using std::wstring;
        using namespace std::literals;

        class Api
        {
        public:
            const static string SPOTIFY_API_URL;
            std::chrono::milliseconds SYNC_INTERVAL = 1000ms;
            
            enum CacheIdx
            {
                Auth = 0,
                History,
                Playback,
                Devices,
            };

        public:
            Api(const string &client_id, const string &client_secret, int port);
            virtual ~Api();

            bool init();
            void shutdown();
            void resync_caches();

            template<class T>
            void start_listening(T *o);
            template<class T>
            void stop_listening(T *o);

            const DevicesList& get_available_devices() const;
            const PlaybackState& get_playback_state() const;
            ArtistsCollection get_artists();
            AlbumsCollection get_albums(const string &artist_id);
            PlaylistsCollection get_playlists();
            std::map<string, SimplifiedTrack> get_tracks(const string &album_id);

            void start_playback(const string &album_id, const string &track_id);
            void skip_to_next();
            void skip_to_previous();
            void toggle_shuffle(bool is_on);
            void set_playback_volume(int volume_percent);
            bool transfer_playback(const string &device_id, bool start_playing = false);

        protected:
            void launch_sync_worker();
            void shutdown_sync_worker();

        private:
            std::unique_ptr<httplib::Client> endpoint;
            std::shared_ptr<spdlog::logger> logger;
            size_t playback_observers;

            string client_id, client_secret;
            int port;

            std::mutex sync_worker_mutex;
            bool is_worker_listening;

            std::vector<std::unique_ptr<ICachedValue>> cache;
        };
        
        template<class T>
        void Api::start_listening(T *o)
        {
            ObserverManager::subscribe<T>(o);

            if (std::is_same<T, PlaybackObserver>::value)
            {
                bool is_playback_active = ++playback_observers > 0;
                for (auto idx: {Playback, Devices})
                    cache[idx]->set_enabled(is_playback_active);
            }
        }

        template<class T>
        void Api::stop_listening(T *o)
        {
            if (std::is_same<T, PlaybackObserver>::value)
            {
                bool is_playback_active = --playback_observers > 0;
                for (auto idx: {Playback, Devices})
                    cache[idx]->set_enabled(is_playback_active);
            }
            ObserverManager::unsubscribe<T>(o);
        }
    }
}

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66