#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A

#include "stdafx.h"
#include "spotify/api.hpp"
#include "spotify/auth.hpp"
#include "ui/panel.hpp"
#include "ui/player.hpp"
#include "utils.hpp"

namespace spotifar {

class plugin:
    public config::config_observer,
    public spotify::auth_observer
{
public:
    plugin();
    virtual ~plugin();

    void start();
    void shutdown();

    void update_panel_info(OpenPanelInfo *info);
    intptr_t update_panel_items(GetFindDataInfo *info);
    void free_panel_items(const FreeFindDataInfo *info);
    intptr_t select_item(const SetDirectoryInfo *info);
    intptr_t process_input(const ProcessPanelInputInfo *info);

protected:
    void launch_sync_worker();
    void shutdown_sync_worker();

    void launch_librespot(const string &access_token);
    
    void check_global_hotkeys();
    void check_librespot_messages();

    virtual void on_global_hotkeys_setting_changed(bool is_enabled);
    virtual void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys);
    virtual void on_logging_verbocity_changed(bool is_verbose);
    virtual void on_auth_status_changed(const spotify::auth &auth);

private:
    std::mutex sync_worker_mutex;
    std::atomic<bool> is_worker_listening = false;

    utils::tasks_queue background_tasks;
    spotify::api api;
    ui::Panel panel;
    ui::player player;
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE m_hChildStd_OUT_Rd = NULL;
    HANDLE m_hChildStd_OUT_Wr = NULL;
};

} // namespace spotifar

#endif //PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A