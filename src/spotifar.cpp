#include "spotifar.hpp"
#include "lng.hpp"
#include "config.hpp"
#include "plugin.h"
#include "ui/events.hpp"
#include "ui/panel.hpp"
#include "ui/dialogs/menus.hpp"

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
extern "C" void WINAPI GetGlobalInfoW(GlobalInfo *info)
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
extern "C" void WINAPI GetPluginInfoW(PluginInfo *info)
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
extern "C" void WINAPI SetStartupInfoW(const PluginStartupInfo *info)
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
extern "C" HANDLE WINAPI OpenW(const OpenInfo *info)
{
    if (auto plugin_ptr = plugin_weak_ptr.lock())
        return new ui::panel(plugin_ptr);
        
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
    return new ui::panel(plugin_ptr);
}

/// @brief https://api.farmanager.com/ru/exported_functions/closepanelw.html
extern "C" void WINAPI ClosePanelW(const ClosePanelInfo *info)
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
extern "C" void WINAPI GetOpenPanelInfoW(OpenPanelInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        panel->update_panel_info(info);
}

/// @brief https://api.farmanager.com/ru/structures/getfinddatainfo.html
extern "C" intptr_t WINAPI GetFindDataW(GetFindDataInfo *info)
{
    // plugin does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;

    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->update_panel_items(info);
    
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/freefinddataw.html 
extern "C" void WINAPI FreeFindDataW(const FreeFindDataInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        panel->free_panel_items(info);
}

/// @brief https://api.farmanager.com/ru/exported_functions/setdirectoryw.html
extern "C" intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
    
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->select_directory(info);
    
    return FALSE;
}

static bool is_plugin_active()
{
    if (auto plugin = plugin_weak_ptr.lock())
    {
        if (auto pinfo = utils::far3::panels::get_info(PANEL_ACTIVE))
            return pinfo->OwnerGuid == MainGuid;
    }
    return false;
}

/// @brief https://api.farmanager.com/ru/exported_functions/processconsoleinputw.html
extern "C" intptr_t WINAPI ProcessConsoleInputW(ProcessConsoleInputInfo *info)
{
    namespace keys = utils::keys;

    if (const auto &key_event = info->Rec.Event.KeyEvent; key_event.bKeyDown)
    {
        auto key = keys::make_combined(key_event);
        if (key == keys::i + keys::mods::ctrl)
        {
            if (is_plugin_active())
            {
                ui::events::show_filters_menu();
                return TRUE;
            }
        }
        else if (key == VK_F7 + keys::mods::alt)
        {
            if (is_plugin_active())
            {
                ui::events::show_search_dialog();
                return TRUE;
            }
        }
    }
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/processpanelinputw.html
extern "C" intptr_t WINAPI ProcessPanelInputW(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    if (const auto &key_event = info->Rec.Event.KeyEvent; key_event.bKeyDown)
    {
        switch (keys::make_combined(key_event))
        {
            case keys::tilde + keys::mods::alt:
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
extern "C" intptr_t WINAPI CompareW(const CompareInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
        return panel->compare_items(info);
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/processpaneleventw.html
extern "C" intptr_t WINAPI ProcessPanelEventW(const ProcessPanelEventInfo *info)
{
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/configurew.html 
extern "C" intptr_t WINAPI ConfigureW(const ConfigureInfo *info)
{
    return ui::show_settings_menu();
}

/// @brief https://api.farmanager.com/ru/exported_functions/processsynchroeventw.html
extern "C" intptr_t WINAPI ProcessSynchroEventW(const ProcessSynchroEventInfo *info)
{
    if (info->Event == SE_COMMONSYNCHRO)
    {
        far3::synchro_tasks::process((intptr_t)info->Param);
        return FALSE;
    }
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/analysew.html 
extern "C" HANDLE WINAPI AnalyseW(const AnalyseInfo *info)
{
    // unfinished experiments, not for the alpha release
    return NULL;
}

/// @brief https://api.farmanager.com/ru/exported_functions/getfilesw.html.
/// The function is also called when file on the panel is being copied to the other panel
extern "C" intptr_t WINAPI GetFilesW(GetFilesInfo *info)
{
    if (auto panel = static_cast<ui::panel*>(info->hPanel))
    {
        // for some reason if I use here "show_waiting", on the `config::ps_info.DialogInit`
        // it immediately runs a second execution of this method, which leads to hanging waiting;
        // replaced it with a simple version of waiting splash dialog
        ui::show_simple_waiting(MWaitingRequestLyrics);

        const auto &files = panel->get_items(info);

        if (files.size() == info->ItemsNumber)
        {
            for (size_t i = 0; i < info->ItemsNumber; i++)
            {
                if (files[i].empty()) continue;

                std::filesystem::path filepath = utils::format(L"{}\\{}.txt", info->DestPath, info->PanelItem[i].FileName);
                if (auto fout = std::ofstream(filepath, std::ios::trunc))
                    fout << utils::utf8_encode(files[i]);
            }
        }
        else
        {
            log::global->warn("The list of files gotten does not match the list of "
                "items requested: {}, {}", info->ItemsNumber, files.size());
        }

        return TRUE;
    }
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/deletefilesw.html
extern "C" intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo *info)
{
    return FALSE;
}

/// @brief https://api.farmanager.com/ru/exported_functions/exitfarw.html
extern "C" void WINAPI ExitFARW(const ExitInfo *info)
{
}

} // namespace spotifar