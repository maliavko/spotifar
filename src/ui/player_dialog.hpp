#ifndef PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3
#define PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3

#include "stdafx.h"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        using spotify::DevicesList;
        using spotify::PlaybackState;
        using spotify::Track;
        using spotify::Context;
        using spotify::Actions;
        
        class PlayerDialog:
            public spotify::PlaybackObserver,
            public spotify::BasicApiObserver
        {
        public:
            PlayerDialog(spotify::Api &api);
            virtual ~PlayerDialog();

            bool show();
            bool hide(bool close_ui = true);

            inline bool is_visible() const { return visible; }
        
            // control even hndlers
            bool on_skip_to_next_btn_click(void *empty);
            bool on_skip_to_previous_btn_click(void *empty);
            bool on_play_btn_click(void *empty);
            bool on_devices_item_selected(void *dialog_item);
            bool on_input_received(void *input_record);

            // control styles
            bool on_playback_control_style_applied(void *dialog_item_colors);
            bool on_track_bar_style_applied(void *dialog_item_colors);
            bool on_inactive_control_style_applied(void *dialog_item_colors);

        protected:
	        friend intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2);
            bool handle_dlg_proc_event(intptr_t msg_id, intptr_t control_id, void *param);

            // api even handlers
            virtual void on_playback_sync_finished(const std::string &err_msg);
            virtual void on_devices_changed(const DevicesList &devices);
            virtual void on_track_changed(const Track &track);
            virtual void on_track_progress_changed(unsigned int duration, unsigned int progress);
            virtual void on_volume_changed(unsigned int volume);
            virtual void on_shuffle_state_changed(bool shuffle_state);
            virtual void on_repeat_state_changed(const std::string &repeat_state);
            virtual void on_state_changed(bool is_playing);
            virtual void on_context_changed(const Context &ctx);
            virtual void on_permissions_changed(const Actions &actions);

            // helpers
            intptr_t set_control_text(int control_id, const std::wstring &text);
            intptr_t set_control_enabled(int control_id, bool is_enabled);

        private:
            spotify::Api& api;
            HANDLE hdlg;
            bool visible = false;
            bool are_dlg_events_suppressed = true;
            
            friend struct DlgEventsSuppressor;
        };
    }
}

#endif //PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3