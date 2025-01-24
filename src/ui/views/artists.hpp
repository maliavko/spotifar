#ifndef ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
#define ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0
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
        using namespace spotify;    

        class ArtistsView: public View
        {
        public:
            inline static const std::string ID = "artists";

        public:
            ArtistsView(spotify::Api *api);

            virtual Items get_items();
            virtual std::shared_ptr<View> select_item(const ItemFarUserData *data);
            virtual void on_panel_updated(OpenPanelInfo *info);
            
            static std::shared_ptr<ArtistsView> create_view(spotify::Api *api);

        protected:
            void next_page();

        private:
            ArtistsT artists;
            spotify::Api *api;
            std::generator<ArtistsT> paginator;
            std::generator<ArtistsT>::iterator paginator_it;
        };
    }
}
#endif // ARTISTS_HPP_1C218DEB_4A4E_4194_8E41_D795A83062B0