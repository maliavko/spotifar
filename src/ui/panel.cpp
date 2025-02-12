#include "stdafx.h"
#include "ui/panel.hpp"
#include "ui/views/root.hpp"
#include "ui/views/artists.hpp"
#include "ui/views/artist.hpp"
#include "ui/views/album.hpp"
#include "ui/views/playlists.hpp"
#include "ui/views/playlist.hpp"

namespace spotifar { namespace ui {

struct far_user_data
{
    string id;
};

Panel::Panel(spotify::api &api):
    api(api)
{
    // TODO: process correctly selected item on the panel
    // https://api.farmanager.com/ru/structures/pluginpanelitem.html
    // PPIF_SELECTED
    ObserverManager::subscribe<ui_events_observer>(this);
}

Panel::~Panel()
{
    ObserverManager::unsubscribe<ui_events_observer>(this);
    view = nullptr;
}

void Panel::update_panel_info(OpenPanelInfo *info)
{
    info->StructSize = sizeof(*info);
    info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;

    // wchar_t FileName[MAX_PATH];
    // config::ps_info.fsf->MkTemp(FileName, std::size(FileName), L"");

    // static InfoPanelLine lines[3]{
    //     { L"Test0", L"Data0" },
    //     { L"Test1", L"Data1" },
    //     { L"Test2", L"Data2" },
    // };

    // info->InfoLines = lines;
    // info->InfoLinesNumber = 3;

    // FAR has a special logic when ".." folder is hit in the panel:
    // if CurDir is empty, it closes the plugin's panel. As plugin does not operate with
    // folders, but spotify items, just in case the current view name is handed over,
    // which equals empty string for the root views
    info->CurDir = view->get_name().c_str();

    // filling the panel top title label
    static wchar_t title[MAX_PATH];
    config::fsf.sprintf(title, L" %s: %s ", utils::far3::get_text(MPluginUserName), info->CurDir);
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
            kbl[j].Text = kbl[j].LongText = utils::far3::get_text(fkeys[i + 2]);
        }
        else
        {
            kbl[j].Text = kbl[j].LongText = L"";
        }
    }

    info->KeyBar = &kbt;

    if (view)
        view->update_panel_info(info);
}

intptr_t Panel::update_panel_items(GetFindDataInfo *info)
{
    const auto &items = view->get_items();

    auto *panel_item = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items.size());
    if (panel_item)
    {
        for (size_t idx = 0; idx < items.size(); idx++)
        {
            auto &item = items[idx];

            //if (item.is_current)
            //    info->CurrentItem = idx;
            
            memset(&panel_item[idx], 0, sizeof(PluginPanelItem));
            panel_item[idx].FileAttributes = item.file_attrs;
            panel_item[idx].FileSize = item.duration;
            panel_item[idx].Flags = PPIF_PROCESSDESCR;
            panel_item[idx].FileName = _wcsdup(item.name.c_str());
            panel_item[idx].Description = NULL; //_wcsdup(item.description.c_str());
            panel_item[idx].UserData.Data = new far_user_data(item.id);
            panel_item[idx].UserData.FreeData = free_user_data;
        }

        info->PanelItem = panel_item;
        info->ItemsNumber = items.size();

        // working
        // size_t size = config::ps_info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, 0, 0);
        // FarPanelDirectory *PPI=(FarPanelDirectory*)malloc(size);
        // if (PPI)
        // {
        //     config::ps_info.PanelControl(PANEL_ACTIVE,FCTL_GETPANELDIRECTORY, size, PPI);
            
        //     free(PPI);
        // }

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
    delete static_cast<const far_user_data*>(UserData);
}

void Panel::change_view(std::shared_ptr<ui::view> view)
{
    utils::log::global->debug("A panel view has been changed, {}", utils::to_string(view->get_name()));
    this->view = view;
}

void Panel::show_root_view()
{
    return change_view(std::make_shared<root_view>(&api));
}

void Panel::show_artists_view()
{
    return change_view(std::make_shared<artists_view>(&api));
}

void Panel::show_artist_view(const spotify::artist &artist)
{
    return change_view(std::make_shared<artist_view>(&api, artist));
}

void Panel::show_album_view(const spotify::artist &artist, const spotify::album &album)
{
    return change_view(std::make_shared<album_view>(&api, artist, album));
}

void Panel::show_playlists_view()
{
    return change_view(std::make_shared<playlists_view>(&api));
}

void Panel::show_playlist_view(const spotify::playlist &playlist)
{
    return change_view(std::make_shared<playlist_view>(&api, playlist));
}

intptr_t Panel::select_item(const SetDirectoryInfo *info)
{
    string item_id = "";
    if (info->UserData.Data != nullptr)
        item_id = reinterpret_cast<const far_user_data*>(info->UserData.Data)->id;

    return view->select_item(item_id);
}

intptr_t Panel::process_input(const ProcessPanelInputInfo *info)
{   
    if (view)
        return view->process_input(info);
    return FALSE;
}

} // namespace ui
} // namespace spotifar