#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "hotkeys_handler.hpp"
#include "spotify/api.hpp"
#include "ui/player.hpp"
#include "ui/dialogs/menus.hpp"
#include "ui/dialogs/search.hpp"
#include "ui/notifications.hpp"

namespace spotifar {

namespace hotkeys = config::hotkeys;
using namespace utils;
using namespace utils::http;
using namespace spotify;
using utils::far3::get_text;
using utils::far3::get_vtext;


/// @brief The requests, which do not require showing splash screen, as they are processed
/// in the background, hidden from user
static const std::set<string> no_splash_requests{
    "/v1/me/player/recently-played",
    "/v1/me/tracks/contains",
};

plugin::plugin(): api(new spotify::api())
{
    hotkeys = std::make_unique<hotkeys_handler>(api);
    notifications = std::make_unique<ui::notifications_handler>(api);
    player = std::make_unique<ui::player>(api, hotkeys.get());
    librespot = std::make_unique<spotifar::librespot>();

    events::start_listening<config::config_observer>(this);
    events::start_listening<spotify::auth_observer>(this);
    events::start_listening<spotify::api_requests_observer>(this);
    events::start_listening<ui::ui_events_observer>(this);
    events::start_listening<librespot_observer>(this);
    
    log::global->info("Spotifar plugin has started, version {}", far3::get_plugin_version());
    
    ui::show_waiting(MWaitingInitSpotify);
    api->start();
    notifications->start();
    
    launch_sync_worker();
}

plugin::~plugin()
{
    try
    {
        player->hide();
        
        shutdown_sync_worker();

        shutdown_librespot_process();

        api->shutdown();
        notifications->shutdown();
    
        events::stop_listening<spotify::auth_observer>(this);
        events::stop_listening<spotify::api_requests_observer>(this);
        events::stop_listening<config::config_observer>(this);
        events::stop_listening<ui::ui_events_observer>(this);
        events::stop_listening<librespot_observer>(this);

        player.reset();
        api.reset();
        notifications.reset();
        hotkeys.reset();
    }
    catch (const std::exception &ex)
    {
        log::global->warn("There is an error while shutting down the "
            "plugin: {}", ex.what());
    }
}

bool plugin::is_player_visible() const
{
    return player->is_visible();
}

spotify::api_ptr_t plugin::get_api()
{ 
    return api;
}

void plugin::launch_sync_worker()
{
    ui::show_waiting(MWaitingInitSyncWorker);

    std::packaged_task<void()> task([this]
    {
        // TODO: temporarily commented for catching flanky bug under debugger
        // try
        // {
            std::lock_guard worker_lock(sync_worker_mutex);
            clock_t::time_point last_tick = clock_t::now();

            while (is_worker_listening)
            {
                const auto &now = clock_t::now();
                const auto &delta = now - last_tick;

                api->tick();
                player->tick();
                librespot->tick();
                hotkeys->tick();

                log::tick(delta);
                ui::waiting::tick(delta);

                std::this_thread::sleep_for(50ms);
                last_tick = now;
            }
        // }
        // catch (const std::exception &ex)
        // {
        //     far3::synchro_tasks::push([ex] {
        //         far3::show_far_error_dlg(MErrorSyncThreadFailed, to_wstring(ex.what()));
        //         ui::events::quit();
        //     }, "sync thread, show error dialog task");
        // }
    });

    is_worker_listening = true;
    std::thread(std::move(task)).detach();
    log::api->info("Plugin's background thread has been launched");
}

void plugin::shutdown_sync_worker()
{
    is_worker_listening = false;
    
    // trying to acquare a sync worker mutex, giving worker time to clean up
    // all the resources
    std::lock_guard worker_lock(sync_worker_mutex);
    log::api->info("Plugin's background thread has been stopped");
}

void plugin::launch_librespot_process(const string &access_token)
{
    if (config::is_playback_backend_enabled())
    {
        ui::show_waiting(MWaitingInitLibrespot);

        if (!librespot->start(access_token))
        {
            shutdown_librespot_process(); // cleaning up the allocated resources if any

            far3::show_far_error_dlg(MErrorLibrespotStartupUnexpected, L"", MShowLogs, []
            {
                far3::panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
            });
        }
    }

    if (!librespot->is_running())
    {
        ui::hide_waiting();
    }
    else
    {
        ui::show_waiting(MWaitingLibrespotDiscover);
    }
}

void plugin::shutdown_librespot_process()
{
    librespot->stop();
}

void plugin::on_playback_backend_setting_changed(bool is_enabled)
{
    if (is_enabled)
    {
        if (const auto &auth_cache = api->get_auth_cache())
            launch_librespot_process(auth_cache->get_access_token());
    }
    else
    {
        shutdown_librespot_process();
    }
}

void plugin::on_playback_backend_configuration_changed()
{
    shutdown_librespot_process();

    std::this_thread::sleep_for(1s);

    if (const auto &auth_cache = api->get_auth_cache())
        launch_librespot_process(auth_cache->get_access_token());
}

void plugin::on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal)
{
    if (auth && !is_renewal) // only if it is not token renewal
    {
        launch_librespot_process(auth.access_token);

        // after first valid authentication we show root view
        ui::events::show_root(api);
    }
}

void plugin::show_player()
{
    player->show();
}

void plugin::show_search_dialog()
{
    ui::search_dialog().run();
}

void plugin::on_request_started(const string &url)
{
    using namespace utils::http;

    // the handler is called only when complex http requests are being initiated, like
    // sync and async multipage collections fetching. In most of the cases it is done when
    // the new view is created and getting populated. So, we show a splash screen and it will
    // get closed automatically when the view initialization is done and the panel is redrawn
    ui::show_waiting(MWaitingRequest);
}

void plugin::on_request_finished(const string &url)
{
    // when any http request is being performed, panel shows a progress splash screen,
    // which gets hidden once all the received panel items are set, but FAR redraws only
    // the active panel, so half of the splash screen stays on the screen. Forcing the
    // passive panel to redraw as well
    //far3::panels::redraw(PANEL_PASSIVE);
    ui::hide_waiting();
}

void plugin::on_request_progress_changed(const string &url, size_t progress, size_t total)
{
    ui::show_waiting(MWaitingItemsProgress, progress, total);
}

void plugin::on_playback_command_failed(const string &message)
{
    utils::far3::show_far_error_dlg(MErrorPlaybackCmdFailed, utils::to_wstring(message));
}

void plugin::on_collection_fetching_failed(const string &message)
{
    utils::far3::show_far_error_dlg(MErrorCollectionFetchFailed, utils::to_wstring(message));
}

void plugin::on_librespot_stopped(bool emergency)
{
    if (emergency)
    {
        far3::synchro_tasks::push([this] {
            far3::show_far_error_dlg(MErrorLibrespotStoppedUnexpectedly, L"", MRelaunch, [this]
            {
                if (auto auth = api->get_auth_cache())
                    launch_librespot_process(auth->get_access_token());
            });
        }, "librespot-unexpected-stop, show error dialog task");
    }
}

void plugin::on_librespot_discovered(const device_t &dev, const device_t &active_dev)
{
    ui::hide_waiting();

    if (active_dev)
    {
        // we are done if the current active device is already Librespot
        if (active_dev.id == dev.id) return;

        // ... if it is some other device - we provide a choice for the user to pick it up or
        // leave the active one untouched
        const auto &message = get_vtext(MTransferPlaybackMessage, active_dev.name, dev.name);
        const wchar_t* msgs[] = {
            get_text(MTransferPlaybackTitle), message.c_str()
        };

        if (config::ps_info.Message(
            &MainGuid, &FarMessageGuid, FMSG_MB_OKCANCEL, nullptr, msgs, std::size(msgs), 0
        ) != 0) // "would you like to transfer a playback?" "Ok" button is "0"
            return;
    }

    log::librespot->info("A librespot process is found, trasferring playback...");
    
    if (auto devices = api->get_devices_cache())
        devices->transfer_playback(dev.id, true);
}

} // namespace spotifar