#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A

#include "stdafx.h"
#include "spotify/api.hpp"
#include "spotify/auth.hpp"
#include "ui/panel.hpp"
#include "ui/player.hpp"
#include "ui/events.hpp"
#include "utils.hpp"
#include "librespot.hpp"

namespace spotifar {

class plugin:
    public config::config_observer, // for catching chnging config settings
    public spotify::auth_observer,  // for launching librespot once credentials were acquared
    public ui::ui_events_observer   // for showin up the player by request
{
public:
    plugin();
    ~plugin();

    void start();
    void shutdown();

    // Far API interface
    void update_panel_info(OpenPanelInfo *info);
    auto update_panel_items(GetFindDataInfo *info) -> intptr_t;
    void free_panel_items(const FreeFindDataInfo *info);
    auto set_directory(const SetDirectoryInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto compare_items(const CompareInfo *info) -> intptr_t;

protected:
    void launch_sync_worker();
    void shutdown_sync_worker();
    
    void check_global_hotkeys();

    // global event handlers
    void on_global_hotkeys_setting_changed(bool is_enabled) override;
    void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys) override;
    void on_logging_verbocity_changed(bool is_verbose) override;
    void on_auth_status_changed(const spotify::auth_t &auth) override;
    void show_player() override;

private:
    std::mutex sync_worker_mutex; // is locked all the way until worker is active
    std::atomic<bool> is_worker_listening = false;

    // a separate tasks queue, so the main thread can execute some code within
    // background thread, like hotkeys check etc.
    utils::tasks_queue background_tasks;

    librespot_handler librespot;
    spotify::api_ptr api;
    ui::panel_ptr panel;
    ui::player_ptr player;
};

} // namespace spotifar

#endif // PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A