#ifndef ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#define ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        using namespace spotify;

        class ArtistView: public View
        {
        public:
            inline static const string ID = "artist";
            
        public:
            ArtistView(Api *api, const Artist &artist);

            virtual Items get_items();
            virtual std::shared_ptr<View> select_item(const ItemFarUserData *data);
            virtual intptr_t process_input(const ProcessPanelInputInfo *info);

            static std::shared_ptr<ArtistView> create_view(Api *api, const Artist &artist);
        private:
            Artist artist;
            Api *api;
        };
    }
}

#endif // ARTIST_HPP_505486D2_3FE3_4140_BF0A_76B2ACB22E61