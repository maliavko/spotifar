#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#pragma

#include "ui/events.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar {

// ideally, it should be in a separate file, but the usage of this class
// is very limited, so not that many dependencies on the class, dose not
// create a lot of hassle to recompile
struct plugin_interface
{
    virtual ~plugin_interface() {}

    virtual bool is_player_visible() const = 0;

    virtual auto get_api() -> spotify::api_ptr_t = 0;
};

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

    bool is_player_visible() const override;
    auto get_api() -> spotify::api_ptr_t override;
protected:
    void launch_sync_worker();
    void shutdown_sync_worker();
    
    void process_win_messages_queue();

    // config handlers
    void on_global_hotkeys_setting_changed(bool is_enabled) override;
    void on_global_hotkey_changed(config::settings::hotkeys_t changed_keys) override;
    void on_logging_verbocity_changed(bool is_verbose) override;

    // auth handler
    void on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal) override;

    // recent releases handler
    void on_releases_sync_finished(const spotify::recent_releases_t releases) override;
    
    // ui events handlers
    void show_player() override;
    void show_search_dialog() override;
    void on_request_started(const string &url) override;
    void on_request_finished(const string &url) override;
    void on_request_progress_changed(const string &url, size_t progress, size_t total) override;
    void on_playback_command_failed(const string &msg) override;
    void on_collection_fetching_failed(const string &msg) override;
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