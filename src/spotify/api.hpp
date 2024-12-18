#ifndef API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "httplib.h"
#include "spotify/items.hpp"
#include "ObserverManager.h"

namespace spotifar
{
    namespace spotify
    {
        using std::string;
        using std::wstring;
        using httplib::Client;
        using namespace std::literals;
        using clock = std::chrono::high_resolution_clock;

        class ApiObserver;
        class Api
        {
        public:
            const static string SPOTIFY_AUTH_URL, SPOTIFY_API_URL;
            std::chrono::milliseconds SYNC_INTERVAL = 1000ms;
            std::chrono::seconds ACCESS_TOKEN_REFRESH_GAP = 10s;

        public:
            Api(const string &client_id, const string &client_secret, int port,
                const string &refresh_token);
            virtual ~Api();

            bool init();
            void shutdown();

            void start_listening(ApiObserver *o, bool is_active = true);
            void stop_listening(ApiObserver *o);
            void resync_caches();

            inline const string& get_refresh_token() const { return refresh_token; }
            inline const DevicesList& get_available_devices() { return *devices; }
            ArtistsCollection get_artist();
            AlbumsCollection get_albums(const string &artist_id);
            std::map<string, SimplifiedTrack> get_tracks(const std::string &album_id);
            PlaybackState get_playback_state();

            void start_playback(const string &album_id, const string &track_id);
            void skip_to_next();
            void skip_to_previous();
            void toggle_shuffle(bool is_on);
            void set_playback_volume(int volume_percent);
            bool transfer_playback(const string &device_id, bool start_playing = false);

        protected:
		    string get_auth_callback_url() const;
            string request_auth_code();
            void request_available_devices(DevicesList &devices_in);
            void launch_sync_worker();
            void shutdown_sync_worker();

            bool authenticate();
            bool update_access_token_with_auth_code(const string &auth_code);
            bool update_access_token_with_refresh_token(const string &refresh_token);
            bool update_access_token(const string &token, const httplib::Params &params);

        private:
            std::unique_ptr<Client> endpoint;
            std::shared_ptr<spdlog::logger> logger;
            std::unordered_set<ApiObserver*> active_observers;

            // cached data
            std::unique_ptr<DevicesList> devices;

            string client_id, client_secret;
            int port;

            string access_token, refresh_token;
            std::chrono::time_point<clock> access_token_expires_at;

            std::mutex sync_worker_mutex;
            bool is_worker_listening;
        };
        
        class ApiObserver: public BaseObserverProtocol
        {
        public:
            virtual void on_playback_updated(const PlaybackState &state) {};
            virtual void on_playback_sync_finished(const std::string &err_msg = "") {};
            virtual void on_devices_changed(const DevicesList &devices) {};
            virtual void on_track_changed(const string &album_id, const string &track_id) {};
        };
    }
}

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66