#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#pragma

#include "stdafx.h"
#include "abstract.hpp"
#include "utils.hpp"
#include "librespot.hpp"
#include "spotify/api.hpp"
#include "spotify/auth.hpp"
#include "spotify/releases.hpp"
#include "ui/player.hpp"
#include "ui/events.hpp"
#include "ui/notifications.hpp"

namespace spotifar {

class plugin:
    public std::enable_shared_from_this<plugin>,
    public plugin_interface,
    public config::config_observer, // for catching changing config settings events
    public spotify::auth_observer, // for launching librespot once credentials were acquaired
    public spotify::releases_observer, // for showing fresh-releases-found notification
    public ui::ui_events_observer // for showing up the player by request
{
public:
    plugin();
    virtual ~plugin();

    std::shared_ptr<plugin> get_ptr() { return shared_from_this(); }
    spotify::api_proxy_ptr get_api() { return api->get_ptr(); }
protected:
    void launch_sync_worker();
    void shutdown_sync_worker();

    void launch_librespot_process(const string &access_token);
    void shutdown_librespot_process();
    
    void process_win_messages_queue();

    // config handlers
    void on_global_hotkeys_setting_changed(bool is_enabled) override;
    void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys) override;
    void on_logging_verbocity_changed(bool is_verbose) override;
    void on_playback_backend_setting_changed(bool is_enabled) override;
    void on_playback_backend_configuration_changed() override;

    // auth handler
    void on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal) override;

    // recent releases handler
    void on_releases_sync_finished(const spotify::recent_releases_t releases) override;
    
    // ui events handlers
    void show_player() override;
private:
    std::mutex sync_worker_mutex; // is locked all the way until worker is active
    std::atomic<bool> is_worker_listening = false;

    // a separate tasks queue, so the main thread can execute some code within
    // background thread, like hotkeys check etc.
    utils::tasks_queue background_tasks;

    std::unique_ptr<librespot_handler> librespot;
    std::unique_ptr<ui::notifications> notifications;
    
    std::unique_ptr<ui::player> player;
    spotify::api_ptr api;
};

} // namespace spotifar

#endif // PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A