#include "plugin.h"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "ui/events.hpp"

namespace spotifar {

using namespace WinToastLib;
using namespace utils;
namespace hotkeys = config::hotkeys;
namespace far3 = utils::far3;


class win_toast_handler : public WinToastLib::IWinToastHandler
{
public:
    win_toast_handler(spotify::api_proxy_ptr api, const spotify::item_id_t &item_id):
        api_proxy(api), item_id(item_id)
        {}
protected:
    // Public interfaces
    void toastActivated() const override {}
    void toastActivated(int actionIndex) const override
    {
        log::global->debug("Button clicked: {}", actionIndex);

        if (auto api = api_proxy.lock())
        {
            if (actionIndex == 0) // like button clicked
                api->save_tracks({ item_id });
        }
    }
    void toastDismissed(WinToastDismissalReason state) const override {}
    void toastFailed() const override {}
private:
    spotify::api_proxy_ptr api_proxy;
    spotify::item_id_t item_id;
};


plugin::plugin(): api(new spotify::api())
{
    panel = std::make_unique<ui::panel>(api);
    player = std::make_unique<ui::player>(api);
    librespot = std::make_unique<librespot_handler>(api);

    utils::events::start_listening<config::config_observer>(this);
    utils::events::start_listening<spotify::auth_observer>(this);
    utils::events::start_listening<ui::ui_events_observer>(this);

    // we mark the listener as a weak one, as it does not require frequent updates
    utils::events::start_listening<ui::playback_observer>(this, true);
}

plugin::~plugin()
{
    utils::events::stop_listening<spotify::auth_observer>(this);
    utils::events::stop_listening<config::config_observer>(this);
    utils::events::stop_listening<ui::ui_events_observer>(this);
    utils::events::stop_listening<ui::playback_observer>(this, true);

    panel.reset();
    player.reset();
    api.reset();
    librespot.reset();
}

void plugin::start()
{
    log::global->info("Spotifar plugin has started, version {}", far3::get_plugin_version());
    
    // TODO: what if not initialized?
    if (api->start())
        ui::events::show_root(api);

    on_global_hotkeys_setting_changed(config::is_global_hotkeys_enabled());
    
    // initializing win toast notifications library
    WinToast::instance()->setAppName(PLUGIN_NAME);
    WinToast::instance()->setAppUserModelId(
        WinToast::configureAUMI(PLUGIN_AUTHOR, PLUGIN_NAME));

    WinToast::WinToastError error;
    if (!WinToast::instance()->initialize(&error))
        utils::far3::show_far_error_dlg(MErrorWinToastStartupUnexpected, WinToast::strerror(error));
    
    launch_sync_worker();
}

void plugin::shutdown()
{
    WinToast::instance()->clear();

    background_tasks.clear_tasks();

    shutdown_sync_worker();
    shutdown_librespot_process();

    player->hide();

    api->shutdown();
}

void plugin::update_panel_info(OpenPanelInfo *info)
{
    panel->update_panel_info(info);
}

intptr_t plugin::update_panel_items(GetFindDataInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
        
    return panel->update_panel_items(info);
}

void plugin::free_panel_items(const FreeFindDataInfo *info)
{
    panel->free_panel_items(info);
}

intptr_t plugin::set_directory(const SetDirectoryInfo *info)
{
    // plugins does not use Far's traditional recursive search mechanism
    if (info->OpMode & OPM_FIND)
        return FALSE;
    
    return panel->select_directory(info);
}

intptr_t plugin::process_input(const ProcessPanelInputInfo *info)
{
    namespace keys = utils::keys;

    const auto &key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        switch (keys::make_combined(key_event))
        {
            case keys::q + keys::mods::alt:
            {
                if (!player->is_visible())
                    player->show();
                return TRUE;
            }
        }
    }
    return panel->process_input(info);
}

intptr_t plugin::compare_items(const CompareInfo *info)
{
    return panel->compare_items(info);
}

void plugin::launch_sync_worker()
{
    std::packaged_task<void()> task([this]
    {
        try
        {
            std::lock_guard worker_lock(sync_worker_mutex);
            while (is_worker_listening)
            {
                api->tick();
                player->tick();
                librespot->tick();

                background_tasks.process_all(); // ticking background tasks if any

                process_win_messages_queue();

                std::this_thread::sleep_for(50ms);
            }
        }
        catch (const std::exception &ex)
        {
            far3::synchro_tasks::push([ex]{
                far3::show_far_error_dlg(MErrorSyncThreadFailed, to_wstring(ex.what()));
                far3::panels::quit(PANEL_ACTIVE);
            });
        }
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
    if (!config::is_playback_backend_enabled()) return;

    if (librespot != nullptr && !librespot->is_launched() && !librespot->launch(access_token))
    {
        shutdown_librespot_process(); // cleaning up the allocated resources if any
        far3::show_far_error_dlg(MErrorLibrespotStartupUnexpected, L"", MShowLogs, []
            {
                far3::panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
            });
    }
}
void plugin::shutdown_librespot_process()
{
    if (librespot != nullptr && librespot->is_launched())
        librespot->shutdown();
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
                auto *hotkey = config::get_hotkey(hotkey_id);
                if (hotkey != nullptr && hotkey->first != utils::keys::none)
                {
                    if (!RegisterHotKey(NULL, hotkey_id, hotkey->second | MOD_NOREPEAT, hotkey->first))
                    {
                        log::global->error("There is an error while registering a hotkey: {}",
                            utils::get_last_system_error());
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

void plugin::on_playback_backend_setting_changed(bool is_enabled)
{
    log::global->debug("on_playback_backend_setting_changed, {}", is_enabled);

    if (is_enabled)
    {
        const auto &access_token = api->get_access_token();
        if (!access_token.empty())
            launch_librespot_process(access_token);
    }
    else
        shutdown_librespot_process();
}

void plugin::on_playback_backend_configuration_changed()
{
    log::global->debug("on_playback_backend_configuration_changed");
    
    const auto &access_token = api->get_access_token();
    if (librespot != nullptr && !access_token.empty())
    {
        shutdown_librespot_process();
    
        std::this_thread::sleep_for(std::chrono::seconds(1));

        launch_librespot_process(access_token);
    }
}

void plugin::on_auth_status_changed(const spotify::auth_t &auth)
{
    if (auth.is_valid())
        launch_librespot_process(auth.access_token);
}

void plugin::on_track_changed(const spotify::track_t &track)
{
    if (config::is_track_changed_notification_enabled() && track.is_valid())
        show_now_playing_notification(track);
}

void plugin::show_player()
{
    player->show();
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
                    case hotkeys::play: return api->toggle_playback();
                    case hotkeys::skip_next: return api->skip_to_next();
                    case hotkeys::skip_previous: return api->skip_to_previous();
                    case hotkeys::seek_forward: return player->on_seek_forward_btn_clicked();
                    case hotkeys::seek_backward: return player->on_seek_backward_btn_clicked();
                    case hotkeys::volume_up: return player->on_volume_up_btn_clicked();
                    case hotkeys::volume_down: return player->on_volume_down_btn_clicked();
                    case hotkeys::show_toast:
                    {
                        const auto &pstate = api->get_playback_state(true);
                        if (!pstate.is_empty())
                            return show_now_playing_notification(pstate.item, true);
                        return;
                    }
                }
                return;
            }
        }
    }
}

void plugin::show_now_playing_notification(const spotify::track_t &track, bool show_buttons)
{
    auto img_path = api->get_image(track.album.images[1], track.album.id);
    if (img_path.empty())
        return;
 
    WinToastTemplate toast(WinToastTemplate::ImageAndText02);   

    // image
    auto crop_hint = WinToastTemplate::CropHint::Square;
    if (config::is_notification_image_circled())
        crop_hint = WinToastTemplate::CropHint::Circle;
    toast.setImagePath(img_path, crop_hint);
    
    // if (WinToast::isWin10AnniversaryOrHigher())
    //     toast.setHeroImagePath(L"D:\\tmp2.jpg", false);
    
    // text
    toast.setTextField(track.name, WinToastTemplate::FirstLine);
    toast.setTextField(track.get_artist_name(), WinToastTemplate::SecondLine);
    toast.setAttributionText(L"Content is provided by Spotify service");
    
    // buttons
    if (show_buttons)
    {
        toast.addAction(L"Like");
    }
    
    // attributes
    toast.setDuration(WinToastTemplate::Duration::Short);
    toast.setAudioOption(WinToastTemplate::AudioOption::Silent);
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::DefaultSound);
    
    WinToast::WinToastError error;
    if (WinToast::instance()->showToast(toast, new win_toast_handler(api->get_ptr(), track.id), &error) < 0)
        log::global->error("There is an error showing windows notification, {}",
            utils::to_string(WinToast::strerror(error)));
}

} // namespace spotifar