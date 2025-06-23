#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "playback_handler.hpp"
#include "spotify/api.hpp"
#include "ui/player.hpp"
#include "ui/dialogs/menus.hpp"
#include "ui/dialogs/search.hpp"
#include "ui/notifications.hpp"

namespace spotifar {

namespace hotkeys = config::hotkeys;
using namespace utils;
using namespace utils::http;

/// @brief The requests, which do not require showing splash screen, as they are processed
/// in the background, hidden from user
static const std::set<string> no_splash_requests{
    "/v1/me/player/recently-played",
    "/v1/me/tracks/contains",
};

plugin::plugin(): api(new spotify::api())
{
    player = std::make_unique<ui::player>(api);
    notifications = std::make_unique<ui::notifications>(api);
    playback_device = std::make_unique<playback_handler>(api);

    events::start_listening<config::config_observer>(this);
    events::start_listening<spotify::auth_observer>(this);
    events::start_listening<spotify::releases_observer>(this);
    events::start_listening<spotify::api_requests_observer>(this);
    events::start_listening<ui::ui_events_observer>(this);
    
    log::global->info("Spotifar plugin has started, version {}", far3::get_plugin_version());
    
    ui::show_waiting(MWaitingInitSpotify);
    {
        api->start();
        notifications->start();
    }

    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
    
    launch_sync_worker();
}

plugin::~plugin()
{
    try
    {
        player->hide();

        background_tasks.clear_tasks();

        ui::show_waiting(MWaitingFiniSpotify);
        {
            api->shutdown();
            notifications->shutdown();
        }

        shutdown_sync_worker();
    
        events::stop_listening<spotify::releases_observer>(this);
        events::stop_listening<spotify::auth_observer>(this);
        events::stop_listening<spotify::api_requests_observer>(this);
        events::stop_listening<config::config_observer>(this);
        events::stop_listening<ui::ui_events_observer>(this);

        player.reset();
        api.reset();
        playback_device.reset();
        notifications.reset();
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
        try
        {
            std::lock_guard worker_lock(sync_worker_mutex);
            clock_t::time_point last_tick = clock_t::now();

            while (is_worker_listening)
            {
                const auto &now = clock_t::now();
                const auto &delta = now - last_tick;

                api->tick();
                player->tick();
                playback_device->tick();

                background_tasks.process_all(); // ticking background tasks if any

                process_win_messages_queue();

                log::tick(delta);

                std::this_thread::sleep_for(50ms);
                last_tick = now;
            }
        }
        catch (const std::exception &ex)
        {
            far3::synchro_tasks::push([ex] {
                far3::show_far_error_dlg(MErrorSyncThreadFailed, to_wstring(ex.what()));
                ui::events::quit();
            }, "synch thread, show error dialog task");
        }
    });

    is_worker_listening = true;
    std::thread(std::move(task)).detach();
    log::api->info("Plugin's background thread has been launched");
}

void plugin::shutdown_sync_worker()
{
    ui::show_waiting(MWaitingFiniSyncWorker);

    is_worker_listening = false;
    
    // trying to acquare a sync worker mutex, giving worker time to clean up
    // all the resources
    std::lock_guard worker_lock(sync_worker_mutex);
    log::api->info("Plugin's background thread has been stopped");
}

void plugin::on_global_hotkeys_setting_changed(bool is_enabled)
{
    // the definition of the global hotkeys must be performed in the
    // the same thread, where the keys check is happening. So, here we push
    // enabling function to the background tasks queue
    background_tasks.push_task([is_enabled] {
        log::global->info("Changing global hotkeys state: {}", is_enabled);

        for (int hotkey_id = hotkeys::play; hotkey_id != hotkeys::last; hotkey_id++)
        {
            UnregisterHotKey(NULL, hotkey_id); // first, we unregister all the hotkeys

            if (is_enabled) // then reinitialize those, which are enabled
            {
                if (auto *hotkey = config::get_hotkey(hotkey_id); hotkey && hotkey->first != utils::keys::none)
                {
                    if (!RegisterHotKey(NULL, hotkey_id, hotkey->second | MOD_NOREPEAT, hotkey->first))
                    {
                        log::global->error("There is an error while registering a hotkey `{}`: {}",
                            utils::to_string(keys::vk_to_string(hotkey->first)), utils::get_last_system_error());
                        continue;
                    }
                    log::global->debug("A global hotkey is registered, {}, {}", hotkey->first, hotkey->second);
                }
            }
        }
    });
}

void plugin::on_global_hotkey_changed(config::settings::hotkeys_t changed_keys)
{
    // reinitialize all hotkeys
    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
}

void plugin::on_logging_verbocity_changed(bool is_verbose)
{
    log::enable_verbose_logs(is_verbose);
}

void plugin::on_auth_status_changed(const spotify::auth_t &auth, bool is_renewal)
{
    if (auth && !is_renewal) // only if it is not token renewal
    {
        // after first valid authentication we show root view
        ui::events::show_root(api);
    }
}

void plugin::on_releases_sync_finished(const spotify::recent_releases_t releases)
{
    notifications->show_recent_releases_found(releases);
}

void plugin::show_player()
{
    player->show();
}

void plugin::show_search_dialog()
{
    ui::search_dialog().run();
}

void plugin::process_win_messages_queue()
{
    MSG msg = {0};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_HOTKEY:
            {            
                if (!config::is_global_hotkeys_enabled())
                    return;

                switch (LOWORD(msg.wParam))
                {
                    case hotkeys::play: player->on_play_btn_click(); return;
                    case hotkeys::skip_next: player->on_skip_to_next_btn_click(); return;
                    case hotkeys::skip_previous: player->on_skip_to_previous_btn_click(); return;
                    case hotkeys::seek_forward: return player->on_seek_forward_btn_clicked();
                    case hotkeys::seek_backward: return player->on_seek_backward_btn_clicked();
                    case hotkeys::volume_up: return player->on_volume_up_btn_clicked();
                    case hotkeys::volume_down: return player->on_volume_down_btn_clicked();
                    case hotkeys::show_toast:
                    {
                        const auto &pstate = api->get_playback_state(true);
                        if (!pstate.is_empty() && notifications != nullptr)
                        {
                            far3::synchro_tasks::push([this, pstate] {
                                notifications->show_now_playing(pstate.item, true);
                            }, "notification, show_now_playing");
                        }
                        return;
                    }
                }
                return;
            }
        }
    }
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
    far3::panels::redraw(PANEL_PASSIVE);
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

} // namespace spotifar