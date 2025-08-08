#include "notifications.hpp"
#include "lng.hpp"
#include "utils.hpp"
#include "plugin.h"
#include "spotifar.hpp"
#include "ui/events.hpp"
#include "spotify/interfaces.hpp"

namespace spotifar { namespace ui {

using namespace spotify;
using utils::far3::get_text;
using utils::far3::synchro_tasks::dispatch_event;


namespace notifications
{
    void show_now_playing(const spotify::track_t &track, bool show_buttons)
    {
        dispatch_event(&notifications_observer::show_now_playing, track, show_buttons);
    }

    void show_releases_found(const spotify::recent_releases_t &releases)
    {
        dispatch_event(&notifications_observer::show_releases_found, releases);
    }
}

#if defined(WIN_TOAST_ENABLED)

using namespace WinToastLib;

/// @brief Toasts notifications handler-class, for handling actions
/// performed on the track-changed toast notification
class track_changed_handler:
    public WinToastLib::IWinToastHandler,
    public playback_observer
{
public:
    track_changed_handler(api_weak_ptr_t api, const item_id_t &track_id):
        api_proxy(api), track_id(track_id)
    {
        utils::events::start_listening<spotify::playback_observer>(this, true);
    }

    ~track_changed_handler()
    {
        api_proxy.reset();
        utils::events::stop_listening<spotify::playback_observer>(this, true);
    }

    void set_toast_id(INT64 tid)
    {
        toast_id = tid;
    }
protected:
    // plyback_observer interface
    void on_track_changed(const track_t &track, const track_t &prev_track) override
    {
        // if the previous track is the one, the toasts was shown for, we request to
        // hide it, incase it is still visible and not needed anymore
        if (prev_track && prev_track.id == track_id && toast_id > 0)
            WinToast::instance()->hideToast(toast_id);
    }

    // WinToast interface
    void toastActivated() const override {}
    void toastActivated(const char* response) const override {}
    void toastActivated(int action_idx) const override
    {
        log::global->debug("Notification's button clicked: toast id {}, button idx {}",
            toast_id, action_idx);

        if (auto api = api_proxy.lock())
        {
            auto *library = api->get_library();
            switch (action_idx)
            {
                case 0: // like btn
                    library->save_tracks({ track_id });
                    break;
                case 1: // next btn
                    api->skip_to_next();
                    break;
            }
        }
    }
    void toastDismissed(WinToastDismissalReason state) const override {}
    void toastFailed() const override {}
private:
    api_weak_ptr_t api_proxy;
    item_id_t track_id;
    INT64 toast_id = -1;
};

/// Fresh releases found notification's handler
class fresh_releases_handler: public WinToastLib::IWinToastHandler
{
public:
    fresh_releases_handler(api_weak_ptr_t api): api_proxy(api) {}
    ~fresh_releases_handler()
    {
        api_proxy.reset();
    }
protected:
    // WinToast interface
    void toastActivated() const override {}
    void toastActivated(const char* response) const override {}
    void toastActivated(int action_idx) const override
    {
        log::global->debug("Notification's button clicked: button idx {}", action_idx);

        ui::events::show_new_releases(api_proxy);
    }
    void toastDismissed(WinToastDismissalReason state) const override {}
    void toastFailed() const override {}
private:
    api_weak_ptr_t api_proxy;
};

#endif

//-----------------------------------------------------------------------------------------------------------------------
bool notifications_handler::start()
{
#if defined(WIN_TOAST_ENABLED)
    // we mark the listener as a weak one, as it does not require frequent updates
    utils::events::start_listening<spotify::playback_observer>(this, true);
    utils::events::start_listening<spotify::releases_observer>(this);
    utils::events::start_listening<notifications_observer>(this);

    try
    {
        // initializing win toast notifications library
        WinToast::instance()->setAppName(PLUGIN_NAME);
        WinToast::instance()->setAppUserModelId(
            WinToast::configureAUMI(PLUGIN_AUTHOR, PLUGIN_NAME));
    
        WinToast::WinToastError error;
        if (!WinToast::instance()->initialize(&error))
        {
            utils::far3::show_far_error_dlg(MErrorWinToastStartupUnexpected, WinToast::strerror(error));
            return false;
        }
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error initializing a WinToast notifications "
            "library: {}", ex.what());
        return false;
    }
#endif
    return true;
}

bool notifications_handler::shutdown()
{
#if defined(WIN_TOAST_ENABLED)
    try
    {
        WinToast::instance()->clear();

        utils::events::stop_listening<spotify::playback_observer>(this, true);
        utils::events::stop_listening<spotify::releases_observer>(this);
        utils::events::stop_listening<notifications_observer>(this);
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error finalizing a WinToast notifications "
            "library: {}", ex.what());
        return false;
    }
#endif
    return true;
}

void notifications_handler::on_track_changed(const track_t &track, const track_t &prev_track)
{
    using utils::far3::actl::is_wnd_in_focus;

#if defined(WIN_TOAST_ENABLED)
    if (WinToast::instance()->isInitialized())
    {
        if (const auto plugin_ptr = get_plugin())
        {
            // do not show a tray notification if the Far window is in focus and player is visible
            bool already_seen = is_wnd_in_focus() && plugin_ptr->is_player_visible();
            
            if (config::is_track_changed_notification_enabled() && track && !already_seen)
                show_now_playing(track, true);
        }
    }
#endif
}

void notifications_handler::on_releases_sync_finished(const spotify::recent_releases_t releases)
{
    show_releases_found(releases);
}

void notifications_handler::show_now_playing(const spotify::track_t &track, bool show_buttons)
{
#if defined(WIN_TOAST_ENABLED)
    if (!WinToast::instance()->isInitialized()) return;

    if (auto api = api_proxy.lock())
    {
        auto *library = api->get_library();
        auto album_img_path = api->get_image(track.album.get_image(), track.album.id);
        auto is_saved = library->is_track_saved(track.id, true);
     
        WinToastTemplate toast(WinToastTemplate::ImageAndText02);
    
        // album image
        auto crop_hint = WinToastTemplate::CropHint::Square;
        if (config::is_notification_image_circled())
            crop_hint = WinToastTemplate::CropHint::Circle;

        if (!album_img_path.empty())
            toast.setImagePath(album_img_path, crop_hint);
        
        // if (WinToast::isWin10AnniversaryOrHigher())
        // {
        //     auto artist = api->get_artist(track.get_artist().id);
        //     auto artist_img_path = api->get_image(artist.get_image(), artist.id);
        //     toast.setHeroImagePath(artist_img_path, true);
        // }
        
        // text
        toast.setTextField(track.name, WinToastTemplate::FirstLine);
        toast.setTextField(track.album.get_artist().name, WinToastTemplate::SecondLine);
        toast.setAttributionText(get_text(MToastSpotifyAttibution));
        
        // buttons
        if (show_buttons)
        {
            toast.addAction(get_text(
                is_saved ? MToastTrackChangedUnlikeBtn : MToastTrackChangedLikeBtn));
            toast.addAction(get_text(MToastTrackChangedNextBtn));
        }
        
        // attributes
        toast.setDuration(WinToastTemplate::Duration::Short);
        toast.setAudioOption(WinToastTemplate::AudioOption::Silent);
        toast.setAudioPath(WinToastTemplate::AudioSystemFile::DefaultSound);
        
        WinToast::WinToastError error;
        auto handler = new track_changed_handler(api->get_ptr(), track.id);
        auto toast_id = WinToast::instance()->showToast(toast, handler, &error);
    
        handler->set_toast_id(toast_id);
    
        if (toast_id < 0)
            log::global->error("There is an error showing track changed notification, {}",
                utils::to_string(WinToast::strerror(error)));
    }
#endif
}

void notifications_handler::show_releases_found(const spotify::recent_releases_t &releases)
{
#if defined(WIN_TOAST_ENABLED)
    if (!WinToast::instance()->isInitialized()) return;

    if (auto api = api_proxy.lock())
    {
        WinToastTemplate toast(WinToastTemplate::Text02);
        
        // first line is Label
        toast.setTextField(get_text(MToastNewReleasesFoundTitle), WinToastTemplate::FirstLine);

        // the second line is the list of artists' names
        std::vector<wstring> artists_names;
        for (const auto &r: releases)
            artists_names.push_back(r.get_artist().name);
        toast.setTextField(utils::string_join(artists_names, L", "), WinToastTemplate::SecondLine);

        toast.setAttributionText(get_text(MToastSpotifyAttibution));
        
        // buttons
        toast.addAction(get_text(MToastNewReleasesFoundHaveALookBtn));
        
        // attributes
        toast.setDuration(WinToastTemplate::Duration::Short);
        toast.setAudioOption(WinToastTemplate::AudioOption::Silent);
        toast.setAudioPath(WinToastTemplate::AudioSystemFile::DefaultSound);
        
        WinToast::WinToastError error;
        auto handler = new fresh_releases_handler(api->get_ptr());
        auto toast_id = WinToast::instance()->showToast(toast, handler, &error);
    
        if (toast_id < 0)
            log::global->error("There is an error showing fresh releases notification, {}",
                utils::to_string(WinToast::strerror(error)));
    }
#endif
}

} // namespace ui
} // namespace spotifar