#ifndef PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#define PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        class PlaylistsView: public View
        {
        public:
            inline static const std::string ID = "playlists";
            
        public:
            PlaylistsView(spotify::Api *api);

            virtual Items get_items();
            virtual std::shared_ptr<View> select_item(const ItemFarUserData *data);

            static std::shared_ptr<PlaylistsView> create_view(spotify::Api *api);
        private:
            spotify::Api *api;
        };
    }
}

#endif //PLAYLISTS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F