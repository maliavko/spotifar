#ifndef OBSERVERS_HPP_BA79594B_1DE9_4C81_9F0A_DC8D83671F70
#define OBSERVERS_HPP_BA79594B_1DE9_4C81_9F0A_DC8D83671F70
#pragma once

#include "items.hpp"
#include "ObserverManager.h"

namespace spotifar
{
    namespace spotify
    {
        struct ApiObserver: public BaseObserverProtocol {};

        struct BasicApiObserver: public ApiObserver
        {
            virtual void on_playback_sync_finished(const string &err_msg = "") {};
        };

        struct PlaybackObserver: public ApiObserver
        {
            virtual void on_playback_updated(const PlaybackState &state) {};
            virtual void on_devices_changed(const DevicesList &devices) {};
            //virtual void on_track_changed(const string &album_id, const string &track_id) {};
        };
    }
}

#endif //OBSERVERS_HPP_BA79594B_1DE9_4C81_9F0A_DC8D83671F70