#ifndef CONTROLLER_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#define CONTROLLER_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66
#pragma once

#include "httplib.h"
#include "spotify/items.hpp"

namespace spotifar
{
    namespace api
    {
        using std::string;
        using std::wstring;
        using httplib::Response;

        // TODO: add an expiry time token refreshment
        class Controller
        {
        public:
            const static string SPOTIFY_AUTH_URL;
            const static string SPOTIFY_API_URL;

        public:
            Controller(const string& client_id, const string& client_secret, int port,
                const string& refresh_token);
            virtual ~Controller();

            inline string get_refresh_token() const { return refresh_token; }

            // spotify api
            bool authenticate();
            ArtistsCollection get_artist();
            AlbumsCollection get_albums(const std::string& artist_id);
            TracksCollection get_tracks(const std::string& album_id);
            void start_playback(const std::string& album_id, const std::string& track_id);

        protected:
		    string get_auth_callback_url() const;
            string request_auth_code();

            bool update_access_token_with_auth_code(const string& auth_code);
            bool update_access_token_with_refresh_token(const string& refresh_token);
            bool update_access_token(const string& token, const httplib::Params& params);


        private:
            httplib::Client api;

            string client_id;
            string client_secret;
            int port;

            string access_token;
            std::time_t access_token_expires_at;
            string refresh_token;
        };
    }
}

#endif //CONTROLLER_HPP_DFF0C34C_5CB3_4F4E_B23A_906584C67C66