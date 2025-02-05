#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A

#include "stdafx.h"
#include "spotify/api.hpp"
#include "ui/panel.hpp"
#include "ui/player_dialog.hpp"

namespace spotifar
{
    class Plugin
    {
    public:
        Plugin();
        virtual ~Plugin();

        void start();
        void shutdown();

        void update_panel_info(OpenPanelInfo *info);
        intptr_t update_panel_items(GetFindDataInfo *info);
        void free_panel_items(const FreeFindDataInfo *info);
        intptr_t select_item(const SetDirectoryInfo *info);
        intptr_t process_input(const ProcessPanelInputInfo *info);

    protected:
        void launch_sync_worker();
        void shutdown_sync_worker();
        void check_global_hotkeys();

    private:
        std::mutex sync_worker_mutex;
        std::atomic<bool> is_worker_listening = false;

        spotify::Api api;
        ui::Panel panel;
        ui::PlayerDialog player;
    };
}

#endif //PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A