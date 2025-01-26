#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "observers.hpp"
#include "interfaces.hpp"
#include "playback.hpp"
#include "devices.hpp"
#include "auth.hpp"
#include "history.hpp"
#include "library.hpp"

namespace spotifar
{
    namespace spotify
    {
        using std::string;
        using std::wstring;

        class Api: public IApi
        {
        public:
            Api();
            virtual ~Api();

            bool init();
            void shutdown();

            template<class T> void start_listening(T *o);
            template<class T> void stop_listening(T *o);

            AlbumsCollection get_albums(const string &artist_id);
            PlaylistsCollection get_playlists();
            std::map<string, SimplifiedTrack> get_tracks(const string &album_id);
            inline const DevicesList& get_available_devices() { return devices->get(); }
            inline const PlaybackState& get_playback_state() { return playback->get(); }
            inline virtual bool is_authenticated() const { return auth->is_authenticated(); }
            inline virtual size_t get_playback_observers_count() const { return playback_observers; }

            inline virtual httplib::Client& get_client() { return client; }
            inline virtual BS::thread_pool& get_thread_pool() { return pool; }
            LibraryCache& get_library() { return *library; }

            // NOTE: no args means "resume"
            void start_playback(const string &context_uri = "", const string &track_uri = "",
                                int position_ms = 0, const string &device_id = "");
            void start_playback(const SimplifiedAlbum &album, const SimplifiedTrack &track);
            void start_playback(const SimplifiedPlaylist &playlist, const SimplifiedTrack &track);
            void pause_playback(const string &device_id = "");
            void skip_to_next(const string &device_id = "");
            void skip_to_previous(const string &device_id = "");
            void seek_to_position(int position_ms, const string &device_id = "");
            void toggle_shuffle(bool is_on, const string &device_id = "");
            void set_repeat_state(const std::string &mode, const string &device_id = "");
            void set_playback_volume(int volume_percent, const string &device_id = "");
            void transfer_playback(const string &device_id, bool start_playing = false);

        protected:
            void launch_sync_worker();
            void shutdown_sync_worker();

        private:
            BS::thread_pool pool;
            httplib::Client client;
            size_t playback_observers = 0;

            std::shared_ptr<spdlog::logger> logger;
            std::mutex sync_worker_mutex;
            bool is_worker_listening = false;

            // caches
            std::unique_ptr<PlaybackCache> playback;
            std::unique_ptr<DevicesCache> devices;
            std::unique_ptr<AuthCache> auth;
            std::unique_ptr<PlayedHistory> history;
            std::unique_ptr<LibraryCache> library;

            std::vector<ICachedData*> caches;
        };
        
        template<class T>
        void Api::start_listening(T *o)
        {
            ObserverManager::subscribe<T>(o);
            if (std::is_same<T, PlaybackObserver>::value)
                ++playback_observers;
        }

        template<class T>
        void Api::stop_listening(T *o)
        {
            ObserverManager::unsubscribe<T>(o);
            if (std::is_same<T, PlaybackObserver>::value)
                --playback_observers;
        }
    }
}

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66