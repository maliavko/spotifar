#ifndef PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#define PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B
#pragma once

#include "ui/views.hpp"

namespace spotifar
{
    namespace ui
    {
        class Panel
        {
        public:
            Panel();
            virtual ~Panel();
            
            void gotoRootMenu();
            void gotoArtists();
            void gotoArtist(const std::string& id);
            void gotoAlbum(const std::string& id, const std::string& artist_id);
            void gotoPlaylists();

            void update_panel_info(OpenPanelInfo* info);
            intptr_t update_panel_items(GetFindDataInfo* info, spotify::Api& api);
            void free_panel_items(const FreeFindDataInfo* info);
            intptr_t select_item(const SetDirectoryInfo* info, spotify::Api& api);
        protected:
            static void WINAPI free_user_data(void* const UserData, const FarPanelItemFreeInfo* const Info);
        private:
		    std::unique_ptr<View> view;
        };
    }
}

#endif //PANEL_HPP_A9BFB4E5_8B2C_4800_A7C3_11571828163B