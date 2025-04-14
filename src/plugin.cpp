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


class WinToastHandler : public WinToastLib::IWinToastHandler
{
public:
    WinToastHandler() {}
    // Public interfaces
    void toastActivated() const override {}
    void toastActivated(int actionIndex) const override {
        log::global->debug("Button clicked: {}", actionIndex);
    }
    void toastDismissed(WinToastDismissalReason state) const override {}
    void toastFailed() const override {}
};


plugin::plugin(): api(new spotify::api())
{
    panel = std::make_unique<ui::panel>(api);
    player = std::make_unique<ui::player>(api);
    librespot = std::make_unique<librespot_handler>(api);

    utils::events::start_listening<config::config_observer>(this);
    utils::events::start_listening<spotify::auth_observer>(this);
    utils::events::start_listening<ui::ui_events_observer>(this);
}

plugin::~plugin()
{
    utils::events::stop_listening<spotify::auth_observer>(this);
    utils::events::stop_listening<config::config_observer>(this);
    utils::events::stop_listening<ui::ui_events_observer>(this);

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
    
    if (WinToast::isCompatible())
    {
        WinToast::instance()->setAppName(PLUGIN_NAME);

        const auto aumi = WinToast::configureAUMI(L"mohabouje", L"wintoast", L"wintoastexample", L"20161006");
        WinToast::instance()->setAppUserModelId(aumi);

        if (!WinToast::instance()->initialize())
            utils::far3::show_far_error_dlg(
                MFarMessageErrorStartup, L"Could not initialize a notifications library");
    }
    else
        utils::far3::show_far_error_dlg(
            MFarMessageErrorStartup, L"Your OS is not supported by notifications library");
    
    launch_sync_worker();
}

void plugin::shutdown()
{
    WinToast::instance()->clear();

    background_tasks.clear_tasks();

    shutdown_sync_worker();

    player->hide();

    api->shutdown();
    librespot->shutdown();
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

    auto &key_event = info->Rec.Event.KeyEvent;
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
            case keys::p + keys::mods::alt:
            {
                WinToastTemplate templ;
                templ = WinToastTemplate(WinToastTemplate::ImageAndText02);

                const auto &pstate = api->get_playback_state();
                auto img = pstate.item.album.images[1];

                static std::regex pattern("(^.*://[^/?:]+)(/?.*$)");
        
                std::smatch match;
                std::regex_search(img.url, match, pattern);

                auto cli = httplib::Client(match[1]);

                std::ofstream file("D:\\tmp.png", std::ios_base::binary);

                auto res = cli.Get(match[2],
                    [&](const char *data, size_t data_length)
                    {
                        file.write(data, data_length);
                        return true;
                    });

                file.close();

                auto artist = api->get_artist(pstate.item.artists[0].id);

                img = artist.images[1];
                std::regex_search(img.url, match, pattern);
                auto cli2 = httplib::Client(match[1]);

                std::ofstream file2("D:\\tmp2.png", std::ios_base::binary);

                res = cli2.Get(match[2],
                    [&](const char *data, size_t data_length)
                    {
                        file2.write(data, data_length);
                        return true;
                    });

                file2.close();


                
                // if (WinToast::isWin10AnniversaryOrHigher())
                //     templ.setHeroImagePath(L"D:\\tmp2.png", false);
                
                templ.setAttributionText(L"Spotify content");
                templ.setImagePath(L"D:\\tmp.png", WinToastTemplate::CropHint::Circle);
                
                templ.setTextField(pstate.item.name, WinToastTemplate::FirstLine);
                templ.setTextField(pstate.item.get_artist_name(), WinToastTemplate::SecondLine);
                
                // templ.addAction(L"Yes");
                // templ.addAction(L"No");
                
                // Read the additional options section in the article
                templ.setDuration(WinToastTemplate::Duration::Short);
                templ.setAudioOption(WinToastTemplate::AudioOption::Silent);
                templ.setAudioPath(WinToastTemplate::AudioSystemFile::DefaultSound);
                
                if (WinToast::instance()->showToast(templ, new WinToastHandler()) == -1L)
                {
                    log::global->error("Could not launch your toast notification!");
                }

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
        string exit_msg = "";
        std::lock_guard worker_lock(sync_worker_mutex);

        try
        {
            while (is_worker_listening)
            {
                api->tick();
                player->tick();
                librespot->tick();

                background_tasks.process_all(); // ticking background tasks if any

                check_global_hotkeys();

                std::this_thread::sleep_for(50ms);
            }
        }
        catch (const std::exception &ex)
        {
            // TODO: what if there is an error, but no playback is opened
            exit_msg = ex.what();
            log::api->critical("An exception occured in the background thread: {}", exit_msg);
        }
        
        // TODO: remove and cleanup the code
        // ObserverManager::notify(&BasicApiObserver::on_playback_sync_finished, exit_msg);
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

void plugin::on_auth_status_changed(const spotify::auth_t &auth)
{
    if (auth.is_valid() && !librespot->is_launched() && config::is_playback_backend_enabled())
        if (!librespot->launch(auth.access_token))
        {
            librespot->shutdown(); // cleaning up the allocated resources if any
            utils::far3::show_far_error_dlg(
                MFarMessageErrorStartup, L"There is a problem launching Librespot "
                "process, look at the logs");
        }
}

void plugin::show_player()
{
    player->show();
}

void plugin::check_global_hotkeys()
{
    if (!config::is_global_hotkeys_enabled())
        return;

    // peeking the messages from the thread's queue for the hotkey's ones
    // and processing them
    MSG msg = {0};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_HOTKEY)
    {
        switch (LOWORD(msg.wParam))
        {
            case hotkeys::play: return api->toggle_playback();
            case hotkeys::skip_next: return api->skip_to_next();
            case hotkeys::skip_previous: return api->skip_to_previous();
            case hotkeys::seek_forward: return player->on_seek_forward_btn_clicked();
            case hotkeys::seek_backward: return player->on_seek_backward_btn_clicked();
            case hotkeys::volume_up: return player->on_volume_up_btn_clicked();
            case hotkeys::volume_down: return player->on_volume_down_btn_clicked();
        }
    }
}

} // namespace spotifar