#include "notifications.hpp"
#include "spotify/common.hpp"
#include "lng.hpp"
#include "utils.hpp"
#include "ui/events.hpp"
#include "spotifar.hpp"

namespace spotifar { namespace ui {

using namespace WinToastLib;
using namespace spotify;
using utils::far3::get_text;

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
    void toastActivated(int action_idx) const override
    {
        log::global->debug("Notification's button clicked: toast id {}, button idx {}",
            toast_id, action_idx);

        if (auto api = api_proxy.lock())
        {
            switch (action_idx)
            {
                case 0: // like btn
                    api->save_tracks({ track_id });
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

bool notifications::start()
{
    // we mark the listener as a weak one, as it does not require frequent updates
    utils::events::start_listening<spotify::playback_observer>(this, true);

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
    return true;
}

bool notifications::shutdown()
{
    try
    {
        WinToast::instance()->clear();

        utils::events::stop_listening<spotify::playback_observer>(this, true);
    }
    catch (const std::exception &ex)
    {
        log::global->error("There is an error finalizing a WinToast notifications "
            "library: {}", ex.what());
        return false;
    }
    return true;
}

void notifications::on_track_changed(const track_t &track, const track_t &prev_track)
{
    using utils::far3::actl::is_wnd_in_focus;

    if (WinToast::instance()->initialize())
    {
        if (const auto plugin_ptr = get_plugin())
        {
            // do not show a tray notification if the Far window is in focus and player is visible
            bool already_seen = is_wnd_in_focus() && plugin_ptr->is_player_visible();
            
            if (config::is_track_changed_notification_enabled() && track && !already_seen)
                show_now_playing(track, true);
        }
    }
}

void notifications::show_now_playing(const spotify::track_t &track, bool show_buttons)
{
    if (!WinToast::instance()->initialize()) return;

    if (auto api = api_proxy.lock())
    {
        auto img_path = api->get_image(track.album.images[1], track.album.id);
        if (img_path.empty())
            return;

        auto is_saved = api->check_saved_track(track.id);
     
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
        toast.setTextField(track.album.get_artist_name(), WinToastTemplate::SecondLine);
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
}

void notifications::show_recent_releases_found(const spotify::recent_releases_t &releases)
{
    if (!WinToast::instance()->initialize()) return;

    if (auto api = api_proxy.lock())
    {
        WinToastTemplate toast(WinToastTemplate::Text02);
        
        // first line is Label
        toast.setTextField(get_text(MToastNewReleasesFoundTitle), WinToastTemplate::FirstLine);

        // the second line is the list of artists' names
        std::vector<wstring> artists_names;
        for (const auto &r: releases)
            artists_names.push_back(r.get_artist_name());
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
}

} // namespace ui
} // namespace spotifar