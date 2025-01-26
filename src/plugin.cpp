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
        }
	}

	Plugin::~Plugin()
	{
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

    intptr_t Plugin::process_input(const ProcessPanelInputInfo *info)
    {
        auto& key_event = info->Rec.Event.KeyEvent;
        if (key_event.bKeyDown)
        {
            int key = utils::far3::input_record_to_combined_key(key_event);
            switch (key)
            {
                case VK_F3:
                {
                    return player.show();
                }
            }
        }
        return panel.process_input(info);
    }
}