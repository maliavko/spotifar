#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#pragma

#include "abstract.hpp"
#include "playback_handler.hpp"
#include "spotify/api.hpp"
#include "spotify/observer_protocols.hpp"
#include "ui/player.hpp"
#include "ui/events.hpp"
#include "ui/notifications.hpp"

namespace spotifar {

class plugin:
    public plugin_interface,
    public config::config_observer,
    public spotify::auth_observer,
    public spotify::releases_observer,
    public spotify::api_requests_observer,
    public ui::ui_events_observer
{
public:
    plugin();
    ~plugin();

    bool is_player_visible() const override { return player->is_visible(); }
    spotify::api_weak_ptr_t get_api() override { return api; }
protected:
    void launch_sync_worker();
    void shutdown_sync_worker();

    void launch_playback_device(const string &access_token);
    void shutdown_playback_device();
    
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
    void on_request_started(const string &url) override;
    void on_request_finished(const string &url) override;
    void on_request_progress_changed(const string &url, size_t progress, size_t total) override;
    void on_playback_command_failed(const string &message) override;
    void on_collection_fetching_failed(const string &message) override;
private:
    std::mutex sync_worker_mutex; // is locked all the way until worker is active
    std::atomic<bool> is_worker_listening = false;

    // a separate tasks queue, so the main thread can execute some code within
    // background thread, like hotkeys check etc.
    utils::tasks_queue background_tasks;

    std::unique_ptr<playback_handler> playback_device;
    std::unique_ptr<ui::notifications> notifications;
    std::unique_ptr<ui::player> player;
    std::shared_ptr<spotify::api> api;
};

} // namespace spotifar

#endif // PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A