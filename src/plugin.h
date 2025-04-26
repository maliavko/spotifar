#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A

#include "stdafx.h"
#include "utils.hpp"
#include "librespot.hpp"
#include "spotify/api.hpp"
#include "spotify/auth.hpp"
#include "ui/panel.hpp"
#include "ui/player.hpp"
#include "ui/events.hpp"

namespace spotifar {

class plugin:
    public config::config_observer, // for catching chnging config settings
    public spotify::auth_observer,  // for launching librespot once credentials were acquaired
    public ui::ui_events_observer,   // for showing up the player by request
    public spotify::playback_observer   // for showing up the windows notification when track has changed
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

    void launch_librespot_process(const string &access_token);
    void shutdown_librespot_process();
    
    void process_win_messages_queue();
    void show_now_playing_notification(const spotify::track_t &track, bool show_buttons = false);

    // config handlers
    void on_global_hotkeys_setting_changed(bool is_enabled) override;
    void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys) override;
    void on_logging_verbocity_changed(bool is_verbose) override;
    void on_playback_backend_setting_changed(bool is_enabled) override;
    void on_playback_backend_configuration_changed() override;

    // auth handler
    void on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal) override;
    
    // playback handlers
    void on_track_changed(const spotify::track_t &track) override;
    
    // panel events handlers
    void show_player() override;
private:
    std::mutex sync_worker_mutex; // is locked all the way until worker is active
    std::atomic<bool> is_worker_listening = false;

    // a separate tasks queue, so the main thread can execute some code within
    // background thread, like hotkeys check etc.
    utils::tasks_queue background_tasks;

    std::unique_ptr<librespot_handler> librespot;
    std::unique_ptr<ui::panel> panel;
    std::unique_ptr<ui::player> player;
    
    spotify::api_ptr api;
};

} // namespace spotifar

#endif // PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A