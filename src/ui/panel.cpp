#include "ui/panel.hpp"
#include "config.hpp"
#include "lng.hpp"

namespace spotifar
{
    namespace ui
    {
        Panel::Panel()
        {
        }

        Panel::~Panel()
        {
            
        }
 
        void Panel::gotoRootMenu()
        {
            view = std::make_unique<RootView>();
        }
    
        void Panel::gotoArtists()
        {
            // TODO: process correctly selected item on the panel
            // https://api.farmanager.com/ru/structures/pluginpanelitem.html
            // PPIF_SELECTED
            view = std::make_unique<ArtistsView>();
        }
    
        void Panel::gotoArtist(const std::string& id)
        {
            view = std::make_unique<ArtistView>(id);
        }
    
        void Panel::gotoAlbum(const std::string& id, const std::string& artist_id)
        {
            view = std::make_unique<AlbumView>(id, artist_id);
        }
    
        void Panel::gotoPlaylists()
        {
            view = std::make_unique<PlaylistsView>();
        }
        
        void Panel::update_panel_info(OpenPanelInfo* info)
        {
            info->StructSize = sizeof(*info);
            info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;

            // FAR has a special logic when ".." folder is hit in the panel:
            // if CurDir is empty, it closes the plugin's panel. As plugin does not operate with
            // folders, but spotify items, just in case the current view name is handed over,
            // which equals empty string for the root view
            info->CurDir = _wcsdup(view->get_name().c_str());

            // filling the panel top title label
            static wchar_t title[MAX_PATH];
            config::FSF.sprintf(title, L" %s: %s", config::get_msg(MPanelTitle), info->CurDir);
            info->PanelTitle = title;

            // updating the labels of command key bar in the down of the screen
            // the approach is copied from Network plugin, every third value represents a label,
            // if it is "0", the label will be empty
            static WORD FKeys[] =
            {
            	VK_F3, 0, 0,  // view
            	VK_F4, 0, MKeyBarF4,  // edit -> show player
            	VK_F5, 0, 0,  // copy
            	VK_F6, 0, 0,  // renmov
            	VK_F7, 0, 0,  // mkfold
            	VK_F8, 0, 0,  // delete
            	VK_F1,SHIFT_PRESSED, 0,  // add archive
            	VK_F2,SHIFT_PRESSED, 0,  // extract
            	VK_F3,SHIFT_PRESSED, 0,  // arccmd
            	VK_F4,SHIFT_PRESSED, 0,  // edit...
            	VK_F5,SHIFT_PRESSED, 0,  // copy in-place
            	VK_F6,SHIFT_PRESSED, 0,  // rename
            	VK_F7,SHIFT_PRESSED, 0,  // <empty>
            	VK_F8,SHIFT_PRESSED, 0,  // delete
            	VK_F1,LEFT_CTRL_PRESSED, 0,  // show left panel
            	VK_F2,LEFT_CTRL_PRESSED, 0,  // show right panel
            	VK_F3,LEFT_CTRL_PRESSED, 0,  // sort by name
            	VK_F4,LEFT_CTRL_PRESSED, 0,  // ..by ext
            	VK_F5,LEFT_CTRL_PRESSED, 0,  // ..by write date
            	VK_F6,LEFT_CTRL_PRESSED, 0,  // ..by size
            	VK_F3,LEFT_ALT_PRESSED, 0,  // alt view
            	VK_F4,LEFT_ALT_PRESSED, 0,  // alt edit
            	VK_F5,LEFT_ALT_PRESSED, 0,  // print
            };

            static KeyBarLabel kbl[std::size(FKeys) / 3];
            static KeyBarTitles kbt = {std::size(kbl), kbl};

            for (size_t j = 0, i = 0; i < std::size(FKeys); i += 3, ++j)
            {
            	kbl[j].Key.VirtualKeyCode = FKeys[i];
            	kbl[j].Key.ControlKeyState = FKeys[i + 1];

            	if (FKeys[i + 2])
            	{
            		kbl[j].Text = kbl[j].LongText = config::get_msg(FKeys[i + 2]);
            	}
            	else
            	{
            		kbl[j].Text = kbl[j].LongText = L"";
            	}
            }

            info->KeyBar = &kbt;
        }
        
        intptr_t Panel::update_panel_items(GetFindDataInfo* info, spotify::Api& api)
        {
            auto items = view->get_items(api);
        
            auto* NewPanelItem = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items.size());
            if (NewPanelItem)
            {
            	for (size_t idx = 0; idx < items.size(); idx++)
            	{
            		auto& item = items[idx];
                    
            		memset(&NewPanelItem[idx], 0, sizeof(PluginPanelItem));
            		NewPanelItem[idx].FileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
            		NewPanelItem[idx].FileName = _wcsdup(item.name.c_str());
            		NewPanelItem[idx].Description = _wcsdup(item.description.c_str());
            		NewPanelItem[idx].UserData.Data = new ItemFarUserData(item.id);
            		NewPanelItem[idx].UserData.FreeData = free_user_data;
            	}

            	info->PanelItem = NewPanelItem;
            	info->ItemsNumber = items.size();

            	return TRUE;
            }

            info->PanelItem = nullptr;
            info->ItemsNumber = 0;
            return FALSE;
        }
    
        void Panel::free_panel_items(const FreeFindDataInfo* info)
        {
            for (size_t idx = 0; idx < info->ItemsNumber; idx++)
            {
                free(const_cast<wchar_t*>(info->PanelItem[idx].FileName));
                free(const_cast<wchar_t*>(info->PanelItem[idx].Description));
            }
            free(info->PanelItem);
        }
        
        void WINAPI Panel::free_user_data(void* const UserData, const FarPanelItemFreeInfo* const Info)
        {
            delete static_cast<const ItemFarUserData*>(UserData);
        }
        
        intptr_t Panel::select_item(const SetDirectoryInfo* info, spotify::Api& api)
        {
            const ItemFarUserData* data = NULL;
            if (info->UserData.Data != NULL)
            	data = static_cast<const ItemFarUserData*>(info->UserData.Data);

            std::string next_view = view->select_item(api, data);
            if (next_view != NONE_VIEW_ID)
            {
                if (next_view == ROOT_VIEW_ID)
                {
                    gotoRootMenu();
                }
                else if (next_view == ARTISTS_VIEW_ID)
                {
                    gotoArtists();
                }
                else if (next_view == PLAYLISTS_VIEW_ID)
                {
                    gotoPlaylists();
                }
                else if (next_view == ARTIST_VIEW_ID)
                {
                    gotoArtist(data->id);
                }
                else if (next_view == ALBUM_VIEW_ID)
                {
                    gotoArtist(data->id);
                }


                return TRUE;
            }
            return FALSE;
        }
    }
}