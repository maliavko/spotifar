#ifndef ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#define ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E
#pragma once

#include "stdafx.h"
#include "view.hpp"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        class RootView: public View
        {
        public:
            const string ID = "/";
        
        public:
            RootView(spotify::api *api);

            virtual Items get_items();
            virtual std::shared_ptr<View> select_item(const ItemFarUserData *data);

            static std::shared_ptr<RootView> create_view(spotify::api *api);

        private:
            spotify::api *api;
        };
    }
}
#endif // ROOT_HPP_8F3B8A10_FD85_4711_9D46_06381760C61E