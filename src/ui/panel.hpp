#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "spotify/api.hpp"
#include "ui/views.hpp"

namespace spotifar
{
    namespace ui
    {
        class Panel
        {
        public:
            Panel(spotify::Api &api);
            virtual ~Panel();
            // TODO: consider having here shutdown/close method to cleanup resources
            
            void gotoRootMenu();

            void update_panel_info(OpenPanelInfo *info);
            intptr_t update_panel_items(GetFindDataInfo *info);
            void free_panel_items(const FreeFindDataInfo *info);
            intptr_t select_item(const SetDirectoryInfo *info);
        protected:
            static void WINAPI free_user_data(void *const UserData, const FarPanelItemFreeInfo *const Info);
        private:
		    std::shared_ptr<View> view;
            spotify::Api &api;
        };
    }
}

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B