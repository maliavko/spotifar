#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A

#include "ui/panel.hpp"
#include "ui/player_dialog.hpp"

#include <plugin.hpp>

namespace spotifar
{
    class Plugin
    {
    public:
        Plugin();
        virtual ~Plugin();
        void shutdown();

        void update_panel_info(OpenPanelInfo* info);
        intptr_t update_panel_items(GetFindDataInfo* info);
        void free_panel_items(const FreeFindDataInfo* info);
        intptr_t select_item(const SetDirectoryInfo* info);
        intptr_t show_player();
        intptr_t hide_player();
    private:
        spotify::Api api;
        std::unique_ptr<ui::Panel> panel;
        std::unique_ptr<ui::PlayerDialog> player;
    };
}

#endif //PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A