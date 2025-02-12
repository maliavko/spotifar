#include "stdafx.h"
#include "config.hpp"
#include "plugin.h"
#include "ui/config_dialog.hpp"

namespace spotifar {

using utils::far3::get_text;

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
    info->Flags = 0;

    if (config::is_added_to_disk_menu())
    {
        static const wchar_t *DiskMenuStrings[1];
        DiskMenuStrings[0] = get_text(MPluginUserName);
        info->DiskMenu.Guids = &MenuGuid;
        info->DiskMenu.Strings = DiskMenuStrings;
        info->DiskMenu.Count = std::size(DiskMenuStrings);
    }

    if (TRUE) // add to plugins menu
    {
        static const wchar_t *PluginMenuStrings[1];
        PluginMenuStrings[0] = get_text(MPluginUserName);
        info->PluginMenu.Guids = &MenuGuid;
        info->PluginMenu.Strings = PluginMenuStrings;
        info->PluginMenu.Count = std::size(PluginMenuStrings);
    }

    // add to plugins configuration menu
    static const wchar_t *PluginCfgStrings[1];
    PluginCfgStrings[0] = get_text(MPluginUserName);
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
        
        // initialize logging system
        utils::log::init();
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        utils::far3::show_far_error_dlg(
            MFarMessageErrorLogInit, utils::utf8_decode(ex.what()));
    }
}

/// @brief https://api.farmanager.com/ru/exported_functions/openw.html 
HANDLE WINAPI OpenW(const OpenInfo *info)
{
    auto p = std::make_unique<plugin>();
    p->start();

    return p.release();
}

/// @brief https://api.farmanager.com/ru/structures/openpanelinfo.html
void WINAPI GetOpenPanelInfoW(OpenPanelInfo *info)
{
    auto &p = *static_cast<plugin*>(info->hPanel);
    p.update_panel_info(info);
}

/// @brief https://api.farmanager.com/ru/structures/getfinddatainfo.html
intptr_t WINAPI GetFindDataW(GetFindDataInfo *info)
{
    if (info->OpMode & OPM_FIND)
        return FALSE;
    
    return static_cast<plugin*>(info->hPanel)->update_panel_items(info);
}

/// @brief https://api.farmanager.com/ru/exported_functions/freefinddataw.html 
void WINAPI FreeFindDataW(const FreeFindDataInfo *info)
{
    return static_cast<plugin*>(info->hPanel)->free_panel_items(info);
}

/// @brief https://api.farmanager.com/ru/exported_functions/setdirectoryw.html
intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo *info)
{
    if (info->OpMode & OPM_FIND)
        return FALSE;
        
    return static_cast<plugin*>(info->hPanel)->select_item(info);
}

/// @brief https://api.farmanager.com/ru/exported_functions/processpanelinputw.html 
intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo *info)
{
    // auto &plugin = *static_cast<Plugin*>(info->hPanel);
    return static_cast<plugin*>(info->hPanel)->process_input(info);
}

/// @brief https://api.farmanager.com/ru/exported_functions/processpaneleventw.html
intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo *info)
{
    auto &p = *static_cast<plugin*>(info->hPanel);
    if (info->Event == FE_CLOSE)
    {
        p.shutdown();
    }

    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/configurew.html 
intptr_t WINAPI ConfigureW(const ConfigureInfo *info)
{
    return ui::config_dialog::show();
}

/// @brief https://api.farmanager.com/ru/exported_functions/closepanelw.html 
void WINAPI ClosePanelW(const ClosePanelInfo *info)
{
    // after auto-variable is destroyed, the unique_ptr will be as well
    std::unique_ptr<plugin>(static_cast<plugin*>(info->hPanel));

    config::write();
    utils::log::fini();
    utils::far3::synchro_tasks::clear();
}

/// @brief https://api.farmanager.com/ru/exported_functions/processsynchroeventw.html
intptr_t WINAPI ProcessSynchroEventW(const ProcessSynchroEventInfo *info)
{
    if (info->Event == SE_COMMONSYNCHRO)
    {
        utils::far3::synchro_tasks::process((intptr_t)info->Param);
        return NULL;
    }
    return NULL;
}

/// @brief https://api.farmanager.com/ru/exported_functions/analysew.html 
HANDLE WINAPI AnalyseW(const AnalyseInfo *info)
{
    // spdlog::debug("HANDLE WINAPI AnalyseW(const AnalyseInfo *info)");
    return NULL;
}

/// @brief https://api.farmanager.com/ru/exported_functions/getfilesw.html.
/// The function is also called when file on the panel is being copied to the other panel
intptr_t WINAPI GetFilesW(GetFilesInfo *info)
{
    spdlog::debug("intptr_t WINAPI GetFilesW(GetFilesInfo *info)");
    
    auto file = std::format(L"{}\\{}.txt", info->DestPath, info->PanelItem[0].FileName);
    std::ofstream fout(file, std::ios::trunc);
    fout << "Test data" << std::endl;
    fout.close();

    return TRUE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/deletefilesw.html
intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)
{
    spdlog::debug("intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)");
    return TRUE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/exitfarw.html 
void WINAPI ExitFARW(const ExitInfo *info)
{
    // spdlog::debug("void WINAPI ExitFARW(const ExitInfo *info)");
}

} // namespace spotifar