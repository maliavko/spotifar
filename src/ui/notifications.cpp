#include "notifications.hpp"
#include "spotify/common.hpp"
#include "lng.hpp"
#include "utils.hpp"

namespace spotifar { namespace ui {

using namespace WinToastLib;
using namespace spotify;
using namespace utils;

/// @brief Toasts notifications handler-class
class win_toast_track_handler:
    public WinToastLib::IWinToastHandler,
    public playback_observer
{
public:
    win_toast_track_handler(api_proxy_ptr api, const item_id_t &track_id):
        api_proxy(api), track_id(track_id)
    {
        events::start_listening<spotify::playback_observer>(this, true);
    }

    ~win_toast_track_handler()
    {
        events::stop_listening<spotify::playback_observer>(this, true);
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
        if (prev_track.is_valid() && prev_track.id == track_id && toast_id > 0)
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
    api_proxy_ptr api_proxy;
    item_id_t track_id;
    INT64 toast_id = -1;
};

bool notifications::start()
{
    // we mark the listener as a weak one, as it does not require frequent updates
    events::start_listening<spotify::playback_observer>(this, true);

    try
    {
        // initializing win toast notifications library
        WinToast::instance()->setAppName(PLUGIN_NAME);
        WinToast::instance()->setAppUserModelId(
            WinToast::configureAUMI(PLUGIN_AUTHOR, PLUGIN_NAME));
    
        WinToast::WinToastError error;
        if (!WinToast::instance()->initialize(&error))
        {
            far3::show_far_error_dlg(MErrorWinToastStartupUnexpected, WinToast::strerror(error));
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

        events::stop_listening<spotify::playback_observer>(this, true);
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
    if (!WinToast::instance()->initialize()) return;

    if (config::is_track_changed_notification_enabled() && track.is_valid())
        show_now_playing(track, true);
}


void notifications::show_now_playing(const spotify::track_t &track, bool show_buttons)
{
    if (!WinToast::instance()->initialize()) return;

    if (auto api = api_proxy.lock())
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
            toast.addAction(L"Next");
        }
        
        // attributes
        toast.setDuration(WinToastTemplate::Duration::Short);
        toast.setAudioOption(WinToastTemplate::AudioOption::Silent);
        toast.setAudioPath(WinToastTemplate::AudioSystemFile::DefaultSound);
        
        WinToast::WinToastError error;
        auto handler = new win_toast_track_handler(api->get_ptr(), track.id);
        auto toast_id = WinToast::instance()->showToast(toast, handler, &error);
    
        handler->set_toast_id(toast_id);
    
        if (toast_id < 0)
            log::global->error("There is an error showing windows notification, {}",
                to_string(WinToast::strerror(error)));
    }
}

} // namespace ui
} // namespace spotifar