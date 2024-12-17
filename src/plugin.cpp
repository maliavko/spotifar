#include "plugin.h"
#include "config.hpp"
#include "ui/player_dialog.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
	auto& cfg = config::Opt;

	Plugin::Plugin():
		api(utils::to_string(cfg.SpotifyClientID), utils::to_string(cfg.SpotifyClientSecret),
			cfg.LocalhostServicePort, utils::to_string(cfg.SpotifyRefreshToken)),
        panel(std::make_unique<ui::Panel>(api)),
        player(std::make_unique<ui::PlayerDialog>(api))
	{
		if (api.init())
            panel->gotoRootMenu();
	}

	Plugin::~Plugin()
	{
		config::set_option(cfg.SpotifyRefreshToken, api.get_refresh_token());

        panel = nullptr;
        player = nullptr;
	}

    void Plugin::shutdown()
    {
        // let far close ui itself, to avoid memory violation
        player->hide(false);
        api.shutdown();
    }
    
    void Plugin::update_panel_info(OpenPanelInfo *info)
    {
        panel->update_panel_info(info);
    }
    
    intptr_t Plugin::update_panel_items(GetFindDataInfo *info)
    {
        return panel->update_panel_items(info);
    }
    
    void Plugin::free_panel_items(const FreeFindDataInfo *info)
    {
        panel->free_panel_items(info);
    }
    
    intptr_t Plugin::select_item(const SetDirectoryInfo *info)
    {
        return panel->select_item(info);
    }

    intptr_t Plugin::show_player()
    {
        return player->show();
    }
}