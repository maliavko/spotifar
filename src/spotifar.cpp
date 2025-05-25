#include "spotifar.hpp"
#include "lng.hpp"
#include "config.hpp"
#include "plugin.h"
#include "ui/dialogs/menus.hpp"
#include "ui/events.hpp"
#include "ui/panel.hpp"

namespace spotifar {

namespace far3 = utils::far3;

/// @brief A global pointer to the plugin instance. The plugin's lifecycle is bound to
/// the opened panels, each of which holds a shared pointer to it. Once all the panels
/// are closed, this pointer loses the ref and plugins gets destroyed
static plugin_weak_ptr_t plugin_weak_ptr;

plugin_ptr_t get_plugin()
{
    return plugin_weak_ptr.lock();
}

/// @brief https://api.farmanager.com/ru/exported_functions/getglobalinfow.html
void WINAPI GetGlobalInfoW(GlobalInfo *info)
{
    info->StructSize = sizeof(GlobalInfo);
    info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 4400, VS_RELEASE);
    info->Version = PLUGIN_VERSION;
    info->Guid = MainGuid;
    info->Title = PLUGIN_NAME;
    info->Description = PLUGIN_DESC;
    info->Author = PLUGIN_AUTHOR;
}

/// @brief https://api.farmanager.com/ru/exported_functions/getplugininfow.html
void WINAPI GetPluginInfoW(PluginInfo *info)
{
    info->StructSize = sizeof(*info);
    info->Flags = PF_NONE;

    if (config::is_added_to_disk_menu())
    {
        static const wchar_t *DiskMenuStrings[1];
        DiskMenuStrings[0] = far3::get_text(MPluginUserName);
        info->DiskMenu.Guids = &MenuGuid;
        info->DiskMenu.Strings = DiskMenuStrings;
        info->DiskMenu.Count = std::size(DiskMenuStrings);
    }

    if (TRUE) // add to plugins menu
    {
        static const wchar_t *PluginMenuStrings[1];
        PluginMenuStrings[0] = far3::get_text(MPluginUserName);
        info->PluginMenu.Guids = &MenuGuid;
        info->PluginMenu.Strings = PluginMenuStrings;
        info->PluginMenu.Count = std::size(PluginMenuStrings);
    }

    // add to plugins configuration menu
    static const wchar_t *PluginCfgStrings[1];
    PluginCfgStrings[0] = far3::get_text(MPluginUserName);
    info->PluginConfig.Guids = &MenuGuid;
    info->PluginConfig.Strings = PluginCfgStrings;
    info->PluginConfig.Count = std::size(PluginCfgStrings);
}

/// @brief https://api.farmanager.com/ru/exported_functions/setstartupinfow.html 
void WINAPI SetStartupInfoW(const PluginStartupInfo *info)
{
    try 
    {
        // initialize the global settings
        config::read(info);
    }
    catch (const std::exception &ex)
    {
        far3::show_far_error_dlg(
            MErrorPluginStartupUnexpected, utils::utf8_decode(ex.what()));
    }
}

/// @brief https://api.farmanager.com/ru/exported_functions/openw.html
HANDLE WINAPI OpenW(const OpenInfo *info)
{
    if (auto plugin_ptr = plugin_weak_ptr.lock())
        return new ui::panel(plugin_ptr->get_api(), plugin_ptr);
        
    // initialize logging system
    log::init();

    if (config::get_client_id().empty())
    {
        far3::show_far_error_dlg(MErrorPluginStartupNoClientID);
        return NULL;
    }

    if (config::get_client_secret().empty())
    {
        far3::show_far_error_dlg(MErrorPluginStartupNoClientSecret);
        return NULL;
    }

    // create and initialize a first plugin's instance
    auto plugin_ptr = std::make_shared<plugin>();

    // saving only a weak pointer as a global one
    plugin_weak_ptr = plugin_ptr;

    // ...and passing a shared pointer to the panel to keep plugin alive
    return new ui::panel(plugin_ptr->get_api(), plugin_ptr);
}

/// @brief https://api.farmanager.com/ru/exported_functions/closepanelw.html
void WINAPI ClosePanelW(const ClosePanelInfo *info)
{
    log::global->debug("Plugin's panel is closed, cleaning resources");

    delete static_cast<ui::panel*>(info->hPanel);

    if (plugin_weak_ptr.expired())
    {
        config::cleanup();
        log::fini();
    }
}

/// @brief https://api.farmanager.com/ru/structures/openpanelinfo.html
void WINAPI GetOpenPanelInfoW(OpenPanelInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        panel->update_panel_info(info);
}

/// @brief https://api.farmanager.com/ru/structures/getfinddatainfo.html
intptr_t WINAPI GetFindDataW(GetFindDataInfo *info)
{
    // plugin does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;

    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->update_panel_items(info);
    
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/freefinddataw.html 
void WINAPI FreeFindDataW(const FreeFindDataInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        panel->free_panel_items(info);
}

/// @brief https://api.farmanager.com/ru/exported_functions/setdirectoryw.html
intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
    
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->select_directory(info);
    
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/processconsoleinputw.html
intptr_t WINAPI ProcessConsoleInputW(ProcessConsoleInputInfo *info)
{
    namespace keys = utils::keys;

    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (keys::make_combined(key_event))
        {
            case keys::i + keys::mods::ctrl:
            {
                // process handlers only in case the plugin is loaded into panel
                if (auto plugin = plugin_weak_ptr.lock())
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/processpanelinputw.html
intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (keys::make_combined(key_event))
        {
            case keys::q + keys::mods::alt:
            {
                ui::events::show_player();
                return TRUE;
            }
        }
    }

    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->process_input(info);
    
    return FALSE;
}

/// @brief  @brief https://api.farmanager.com/ru/exported_functions/comparew.html
intptr_t WINAPI CompareW(const CompareInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->compare_items(info);
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/processpaneleventw.html
intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo *info)
{
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/configurew.html 
intptr_t WINAPI ConfigureW(const ConfigureInfo *info)
{
    return ui::show_settings_menu();
}

/// @brief https://api.farmanager.com/ru/exported_functions/processsynchroeventw.html
intptr_t WINAPI ProcessSynchroEventW(const ProcessSynchroEventInfo *info)
{
    if (info->Event == SE_COMMONSYNCHRO)
    {
        far3::synchro_tasks::process((intptr_t)info->Param);
        return NULL;
    }
    return NULL;
}

/// @brief https://api.farmanager.com/ru/exported_functions/analysew.html 
HANDLE WINAPI AnalyseW(const AnalyseInfo *info)
{
    // TODO: unfinished
    // spdlog::debug("HANDLE WINAPI AnalyseW(const AnalyseInfo *info)");
    return NULL;
}

/// @brief https://api.farmanager.com/ru/exported_functions/getfilesw.html.
/// The function is also called when file on the panel is being copied to the other panel
intptr_t WINAPI GetFilesW(GetFilesInfo *info)
{
    // TODO: unfinished
    // spdlog::debug("intptr_t WINAPI GetFilesW(GetFilesInfo *info)");
    
    // // wchar_t FileName[MAX_PATH];
    // // config::ps_info.fsf->MkTemp(FileName, std::size(FileName), L"");

    // auto file = std::format(L"{}\\{}.txt", info->DestPath, info->PanelItem[0].FileName);
    // std::ofstream fout(file, std::ios::trunc);
    // fout << "Test data" << std::endl;
    // fout.close();

    // return TRUE;

    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/deletefilesw.html
intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)
{
    // TODO: unfinished
    // spdlog::debug("intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)");
    return FALSE;
}

} // namespace spotifar