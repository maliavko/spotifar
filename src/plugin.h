#ifndef PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#define PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A
#pragma

#include "ui/events.hpp"
#include "librespot.hpp"
#include "hotkeys_handler.hpp"
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
    public spotify::api_requests_observer,
    public ui::ui_events_observer,
    public librespot_observer
{
public:
    plugin();
    ~plugin();

    bool is_player_visible() const override;
    auto get_api() -> spotify::api_ptr_t override;
protected:
    void launch_sync_worker();
    void shutdown_sync_worker();

    void launch_librespot_process(const string &access_token);
    void shutdown_librespot_process();

    // config handlers
    void on_playback_backend_setting_changed(bool is_enabled) override;
    void on_playback_backend_configuration_changed() override;

    // auth handler
    void on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal) override;
    
    // ui events handlers
    void show_player() override;
    void show_search_dialog() override;
    void on_request_started(const string &url) override;
    void on_request_finished(const string &url) override;
    void on_request_progress_changed(const string &url, size_t progress, size_t total) override;
    void on_playback_command_failed(const string &message) override;
    void on_collection_fetching_failed(const string &message) override;

    // librespot observer handlers
    void on_librespot_stopped(bool emergency) override;
    void on_librespot_discovered(const spotify::device_t &dev, const spotify::device_t &active_dev) override;
private:
    std::mutex sync_worker_mutex; // is locked all the way until worker is active
    std::atomic<bool> is_worker_listening = false;

    std::unique_ptr<ui::notifications_handler> notifications;
    std::unique_ptr<ui::player> player;
    std::unique_ptr<librespot> librespot;
    std::unique_ptr<hotkeys_handler> hotkeys;
    std::shared_ptr<spotify::api> api;
};

} // namespace spotifar

#endif // PLUGIN_HPP_2419C0DE_F1AD_4D6F_B388_25CC7C8D402A