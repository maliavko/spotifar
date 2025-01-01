#include "plugin.h"
#include "config.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
	Plugin::Plugin():
		api(),
        panel(api),
        player(api)
	{
        // TODO: what if not initialized?
		if (api.init())
        {
            panel.gotoRootMenu();
            //api.start_listening(this);
        }
	}

	Plugin::~Plugin()
	{
        //api.stop_listening(this);
	}

    void Plugin::shutdown()
    {
        // false is given to let far close ui itself, to avoid memory violation
        player.hide(false);
        api.shutdown();
    }
    
    void Plugin::update_panel_info(OpenPanelInfo *info)
    {
        panel.update_panel_info(info);
    }
    
    intptr_t Plugin::update_panel_items(GetFindDataInfo *info)
    {
        return panel.update_panel_items(info);
    }
    
    void Plugin::free_panel_items(const FreeFindDataInfo *info)
    {
        panel.free_panel_items(info);
    }
    
    intptr_t Plugin::select_item(const SetDirectoryInfo *info)
    {
        return panel.select_item(info);
    }

    intptr_t Plugin::show_player()
    {
        return player.show();
    }

    // void Plugin::on_track_changed(const std::string &album_id, const std::string &track_id)
    // {
    //     show_player();
    // }
}