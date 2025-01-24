#ifndef ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#define ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        using std::string;
        using std::wstring;

        class AlbumView: public View
        {
        public:
            inline static const std::string ID = "album";
            
        public:
            AlbumView(spotify::Api *api, const string &album_id, const string &artist_id);

            virtual Items get_items();
            virtual std::shared_ptr<View> select_item(const ItemFarUserData *data);

            static std::shared_ptr<AlbumView> create_view(spotify::Api *api,
                const std::string &album_id, const std::string &artist_id);
        private:
            string album_id;
            string artist_id;
            spotify::Api *api;
        };
    }
}
#endif // ALBUM_HPP_7EB92BC0_400F_44EC_9F55_B1C41E7D3DCB