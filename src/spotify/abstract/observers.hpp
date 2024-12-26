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
            virtual void on_devices_changed(const DevicesList &devices) {};
            virtual void on_track_changed(const Track &track) {};
            virtual void on_track_progress_changed(unsigned int duration, unsigned int progress) {};
            virtual void on_volume_changed(unsigned int volume) {};
            virtual void on_shuffle_state_changed(bool shuffle_state) {};
            virtual void on_repeat_state_changed(const std::string &repeat_state) {};
            virtual void on_playback_state_changed(bool is_playing) {};
        };
    }
}

#endif //OBSERVERS_HPP_BA79594B_1DE9_4C81_9F0A_DC8D83671F70