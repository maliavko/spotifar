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
        
        class ApiProtocol: public BaseObserverProtocol
        {
        public:
            virtual void on_playback_updated(const PlaybackState& state) = 0;
            virtual void on_playback_sync_failed(const std::string& err_msg) = 0;
            virtual void on_devices_changed(const DevicesList& devices) = 0;
        };

        // TODO: to implement a cache of requesting data
        class Api
        {
        public:
            const static string SPOTIFY_AUTH_URL;
            const static string SPOTIFY_API_URL;
            std::chrono::milliseconds SYNC_INTERVAL = std::chrono::milliseconds(1000);

        public:
            Api(const string& client_id, const string& client_secret, int port,
                const string& refresh_token);
            virtual ~Api();

            bool authenticate();
            void shutdown();
            inline string get_refresh_token() const { return refresh_token; }

            void start_listening(ApiProtocol* observer);
            void stop_listening(ApiProtocol* observer);
            void resync_caches();

            ArtistsCollection get_artist();
            AlbumsCollection get_albums(const std::string& artist_id);
            std::map<string, SimplifiedTrack> get_tracks(const std::string& album_id);
            PlaybackState get_playback_state();
            inline const DevicesList& get_available_devices() { return *devices; }

            void start_playback(const std::string& album_id, const std::string& track_id);
            void skip_to_next();
            void skip_to_previous();
            void toggle_shuffle(bool is_on);
            void set_playback_volume(int volume_percent);

        protected:
		    string get_auth_callback_url() const;
            string request_auth_code();
            void request_available_devices(DevicesList& devices_in);

            bool update_access_token_with_auth_code(const string& auth_code);
            bool update_access_token_with_refresh_token(const string& refresh_token);
            bool update_access_token(const string& token, const httplib::Params& params);



        private:
            httplib::Client api;

            // cached data
            std::unique_ptr<DevicesList> devices;

            string client_id;
            string client_secret;
            int port;

            string access_token;
            std::time_t access_token_expires_at;
            string refresh_token;
            bool is_listening;
            std::vector<ApiProtocol*> observers;

            std::mutex m;
            std::condition_variable cv;
            bool is_in_sync_with_api = false;
        };
    }
}

#endif //API_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66