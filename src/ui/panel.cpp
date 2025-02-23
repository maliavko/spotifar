#include "stdafx.h"
#include "ui/panel.hpp"
#include "ui/views/root.hpp"
#include "ui/views/artists.hpp"
#include "ui/views/artist.hpp"
#include "ui/views/album.hpp"
#include "ui/views/playlists.hpp"
#include "ui/views/playlist.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;

struct far_user_data
{
    string id;
};

// the `F` keys, which can be overriden by the nested views
static const std::array<int, 6> refreshable_keys = { VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8 };

Panel::Panel(spotify::api_abstract *api):
    api_proxy(api)
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
    
    info->CurDir = view->get_dir_name();
    info->Flags = OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_USEATTRHIGHLIGHTING;

    // filling in the info lines on the Ctrl+L panel
    const auto &info_lines = view->get_info_lines();
    if (info_lines != nullptr)
    {
        info->InfoLines = info_lines->data();
        info->InfoLinesNumber = info_lines->size();
    }

    // filling the panel top title label
    static wchar_t title[64];
    config::fsf.snprintf(title, std::size(title), L" %s: %s ",
        far3::get_text(MPluginUserName), view->get_title());
    info->PanelTitle = title;

    // every update we clear out all the refreshable `F` keys and fill them up
    // by demand with the overriding info from the nested view
    static KeyBarLabel key_bar_labels[refreshable_keys.size() * 4];
    static KeyBarTitles key_bar = { std::size(key_bar_labels), key_bar_labels };
    info->KeyBar = &key_bar;

    const auto view_key_bar = view->get_key_bar_info();

    size_t idx = 0;
    for (const auto key: refreshable_keys)
        for (const auto mod: { 0, SHIFT_PRESSED, LEFT_ALT_PRESSED, LEFT_CTRL_PRESSED })
        {
            auto &kbl = key_bar_labels[idx++];

            kbl.Key.VirtualKeyCode = key;
            kbl.Key.ControlKeyState = mod;

            if (view_key_bar && view_key_bar->contains(kbl.Key))
                kbl.Text = kbl.LongText = view_key_bar->at(kbl.Key).c_str();
            else
                kbl.Text = kbl.LongText = L"";
        }

    if (view)
        return view->update_panel_info(info);
}

intptr_t Panel::update_panel_items(GetFindDataInfo *info)
{
    const auto items = view->get_items();
    if (items == nullptr)
        return FALSE;

    auto *panel_item = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * items->size());
    if (panel_item == nullptr)
        return FALSE;

    for (size_t idx = 0; idx < items->size(); idx++)
    {
        auto &item = (*items)[idx];
        auto &columns = item.custom_column_data;

        // TODO: what if no memory allocated?
        auto **column_data = (const wchar_t**)malloc(sizeof(wchar_t*) * columns.size());
        for (size_t idx = 0; idx < columns.size(); idx++)
            column_data[idx] = _wcsdup(columns[idx].c_str());
        
        memset(&panel_item[idx], 0, sizeof(PluginPanelItem));
        panel_item[idx].FileAttributes = item.file_attrs;
        panel_item[idx].Flags = PPIF_PROCESSDESCR;
        panel_item[idx].FileName = _wcsdup(item.name.c_str());
        panel_item[idx].Description = _wcsdup(item.description.c_str());
        panel_item[idx].CustomColumnData = column_data;
        panel_item[idx].CustomColumnNumber = item.custom_column_data.size();
        panel_item[idx].UserData.Data = new far_user_data(item.id);
        panel_item[idx].UserData.FreeData = free_user_data;
    }

    info->PanelItem = panel_item;
    info->ItemsNumber = items->size();

    return TRUE;
}

void Panel::free_panel_items(const FreeFindDataInfo *info)
{
    for (size_t i = 0; i < info->ItemsNumber; i++)
    {
        auto &item = info->PanelItem[i];

        free(const_cast<wchar_t*>(item.FileName));
        free(const_cast<wchar_t*>(item.Description));

        for (size_t j = 0; j < info->PanelItem[i].CustomColumnNumber; j++)
            free(const_cast<wchar_t*>(item.CustomColumnData[j]));
        free(const_cast<wchar_t**>(item.CustomColumnData));
    }
    free(info->PanelItem);
}

void WINAPI Panel::free_user_data(void *const user_data, const FarPanelItemFreeInfo *const info)
{
    delete static_cast<const far_user_data*>(user_data);
}

void Panel::change_view(std::shared_ptr<ui::view> view)
{
    this->view = view;
}

void Panel::refresh_panels(const string &item_id)
{
    far3::panels::update(PANEL_ACTIVE);

    size_t item_idx = 0;
    if (!item_id.empty())
        item_idx = view->get_item_idx(item_id) + 1; // 0 index is ".."

    far3::panels::redraw(PANEL_ACTIVE, item_idx);
}

void Panel::show_root_view()
{
    return change_view(std::make_shared<root_view>(api_proxy));
}

void Panel::show_artists_view()
{
    return change_view(std::make_shared<artists_view>(api_proxy));
}

void Panel::show_artist_view(const artist &artist)
{
    return change_view(std::make_shared<artist_view>(api_proxy, artist));
}

void Panel::show_album_view(const artist &artist, const album &album)
{
    change_view(std::make_shared<album_view>(api_proxy, artist, album));
}

void Panel::show_playlists_view()
{
    return change_view(std::make_shared<playlists_view>(api_proxy));
}

void Panel::show_playlist_view(const spotify::playlist &playlist)
{
    return change_view(std::make_shared<playlist_view>(api_proxy, playlist));
}

void Panel::show_recents_view()
{
    //return change_view(std::make_shared<playlist_view>(api_proxy, playlist));
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