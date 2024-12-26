#include "stdafx.h"
#include "ui/panel.hpp"

namespace spotifar
{
    namespace ui
    {
        Panel::Panel(spotify::Api &api):
            api(api)
        {
            // TODO: process correctly selected item on the panel
            // https://api.farmanager.com/ru/structures/pluginpanelitem.html
            // PPIF_SELECTED
        }

        Panel::~Panel()
        {
            view = nullptr;
        }
 
        void Panel::gotoRootMenu()
        {
            view = create_root_view();
        }
        
        void Panel::update_panel_info(OpenPanelInfo *info)
        {
            info->StructSize = sizeof(*info);
            info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;

            // FAR has a special logic when ".." folder is hit in the panel:
            // if CurDir is empty, it closes the plugin's panel. As plugin does not operate with
            // folders, but spotify items, just in case the current view name is handed over,
            // which equals empty string for the root view
            // TODO: should I free this pointer?
            info->CurDir = _wcsdup(view->get_name().c_str());

            // filling the panel top title label
            static wchar_t title[MAX_PATH];
            config::FSF.sprintf(title, L" %s: %s", utils::far3::get_msg(MPluginUserName), info->CurDir);
            info->PanelTitle = title;

            // updating the labels of command key bar in the down of the screen
            // the approach is copied from Network plugin, every third value represents a label,
            // if it is "0", the label will be empty
            static WORD fkeys[] =
            {
            	VK_F3, 0, 0,  // view
            	VK_F4, 0, MKeyBarF4,  // edit -> show player
            	VK_F5, 0, 0,  // copy
            	VK_F6, 0, 0,  // renmov
            	VK_F7, 0, 0,  // mkfold
            	VK_F8, 0, 0,  // delete
            	VK_F1, SHIFT_PRESSED, 0,  // add archive
            	VK_F2, SHIFT_PRESSED, 0,  // extract
            	VK_F3, SHIFT_PRESSED, 0,  // arccmd
            	VK_F4, SHIFT_PRESSED, 0,  // edit...
            	VK_F5, SHIFT_PRESSED, 0,  // copy in-place
            	VK_F6, SHIFT_PRESSED, 0,  // rename
            	VK_F7, SHIFT_PRESSED, 0,  // <empty>
            	VK_F8, SHIFT_PRESSED, 0,  // delete
            	VK_F1, LEFT_CTRL_PRESSED, 0,  // show left panel
            	VK_F2, LEFT_CTRL_PRESSED, 0,  // show right panel
            	VK_F3, LEFT_CTRL_PRESSED, 0,  // sort by name
            	VK_F4, LEFT_CTRL_PRESSED, 0,  // ..by ext
            	VK_F5, LEFT_CTRL_PRESSED, 0,  // ..by write date
            	VK_F6, LEFT_CTRL_PRESSED, 0,  // ..by size
            	VK_F3, LEFT_ALT_PRESSED, 0,  // alt view
            	VK_F4, LEFT_ALT_PRESSED, 0,  // alt edit
            	VK_F5, LEFT_ALT_PRESSED, 0,  // print
            };

            static KeyBarLabel kbl[std::size(fkeys) / 3];
            static KeyBarTitles kbt = {std::size(kbl), kbl};

            for (size_t j = 0, i = 0; i < std::size(fkeys); i += 3, ++j)
            {
            	kbl[j].Key.VirtualKeyCode = fkeys[i];
            	kbl[j].Key.ControlKeyState = fkeys[i + 1];

            	if (fkeys[i + 2])
            	{
            		kbl[j].Text = kbl[j].LongText = utils::far3::get_msg(fkeys[i + 2]);
            	}
            	else
            	{
            		kbl[j].Text = kbl[j].LongText = L"";
            	}
            }

            info->KeyBar = &kbt;
        }
        
        intptr_t Panel::update_panel_items(GetFindDataInfo *info)
        {
            auto items = view->get_items(api);
        
            auto* panel_item = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items.size());
            if (panel_item)
            {
            	for (size_t idx = 0; idx < items.size(); idx++)
            	{
            		auto& item = items[idx];
                    
            		memset(&panel_item[idx], 0, sizeof(PluginPanelItem));
            		panel_item[idx].FileAttributes = item.file_attrs;
                    panel_item[idx].FileSize = item.duration;
            		panel_item[idx].FileName = _wcsdup(item.name.c_str());
            		panel_item[idx].Description = _wcsdup(item.description.c_str());
            		panel_item[idx].UserData.Data = new ItemFarUserData(item.id);
            		panel_item[idx].UserData.FreeData = free_user_data;
            	}

            	info->PanelItem = panel_item;
            	info->ItemsNumber = items.size();

            	return TRUE;
            }

            info->PanelItem = nullptr;
            info->ItemsNumber = 0;
            return FALSE;
        }
    
        void Panel::free_panel_items(const FreeFindDataInfo *info)
        {
            for (size_t idx = 0; idx < info->ItemsNumber; idx++)
            {
                free(const_cast<wchar_t*>(info->PanelItem[idx].FileName));
                free(const_cast<wchar_t*>(info->PanelItem[idx].Description));
            }
            free(info->PanelItem);
        }
        
        void WINAPI Panel::free_user_data(void *const UserData, const FarPanelItemFreeInfo *const Info)
        {
            delete static_cast<const ItemFarUserData*>(UserData);
        }
        
        intptr_t Panel::select_item(const SetDirectoryInfo *info)
        {
            const ItemFarUserData* data = nullptr;
            if (info->UserData.Data != nullptr)
            	data = static_cast<const ItemFarUserData*>(info->UserData.Data);

            std::shared_ptr<View> next_view = view->select_item(api, data);
            if (next_view != nullptr)
            {
                view = next_view;
                return TRUE;
            }
            return TRUE;
        }
    }
}