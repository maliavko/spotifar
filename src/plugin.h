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
    public config::config_observer,
    public spotify::auth_observer,
    public ui::ui_events_observer
{
public:
    plugin();
    ~plugin();

    void start();
    void shutdown();

    auto update_panel_info(OpenPanelInfo *info) -> void;
    auto update_panel_items(GetFindDataInfo *info) -> intptr_t;
    auto free_panel_items(const FreeFindDataInfo *info) -> void;
    auto set_directory(const SetDirectoryInfo *info) -> intptr_t;
    auto process_input(const ProcessPanelInputInfo *info) -> intptr_t;
    auto compare_items(const CompareInfo *info) -> intptr_t;

protected:
    void launch_sync_worker();
    void shutdown_sync_worker();
    
    void check_global_hotkeys();

    // event handlers
    void on_global_hotkeys_setting_changed(bool is_enabled);
    void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys);
    void on_logging_verbocity_changed(bool is_verbose);
    void on_auth_status_changed(const spotify::auth &auth);
    void show_player_dialog();

private:
    std::mutex sync_worker_mutex;
    std::atomic<bool> is_worker_listening = false;
    utils::tasks_queue background_tasks;

    librespot_handler librespot;
    spotify::api api;
    ui::panel panel;
    ui::player player;
};

} // namespace spotifar

#endif //PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A