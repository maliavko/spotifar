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
        
        // TODO: add links to art with redirection to browser
        class PlayerDialog: public spotify::ApiProtocol
        {
        public:
            friend struct DlgEventsSuppressor;

            enum DialogControls
            {
                NO_CONTROL = -1,
                BOX,
                TITLE,
                TRACK_BAR,
                TRACK_TIME,
                TRACK_TOTAL_TIME,
                ARTIST_NAME,
                TRACK_NAME,
                PLAY_BTN,
                PREV_BTN,
                NEXT_BTN,
                LIKE_BTN,
                VOLUME_LABEL,
                REPEAT_BTN,
                SHUFFLE_BTN,
                DEVICES_COMBO,
                TOTAL_ELEMENTS_COUNT,
            };

            static const wchar_t TRACK_BAR_CHAR_UNFILLED = 0x2591;
            static const wchar_t TRACK_BAR_CHAR_FILLED = 0x2588;

            static const int width = 60, height = 10;
            static const int view_x = 2, view_y = 2, view_width = width - 2, view_height = height - 2;
            static const int view_center_x = (view_width + view_x)/2, view_center_y = (view_height + view_y)/2;

        public:
            PlayerDialog(spotify::Api &api);
            virtual ~PlayerDialog();

            bool show();
            bool hide(bool close_ui = true);

            bool is_visible() const { return visible; }

        protected:
	        friend intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2);
            bool handle_dlg_proc_event(intptr_t msg_id, DialogControls control_id, void *param);

            void update_track_bar(int track_total_time = 0, int track_played_time = 0);
            void update_controls_block(const PlaybackState &state);
            void update_track_info(const std::wstring &artist_name = L"", const std::wstring &track_name = L"");
            void update_devices_list(const DevicesList &devices);

            // control even hndlers
            bool on_skip_to_next_btn_click(void *empty);
            bool on_skip_to_previous_btn_click(void *empty);
            bool on_devices_item_selected(void *dialog_item);
            bool on_input_received(void *input_record);

            // control styles
            bool on_playback_control_style_applied(void *dialog_item_colors);
            bool on_track_bar_style_applied(void *dialog_item_colors);
            bool on_inactive_control_style_applied(void *dialog_item_colors);

            // api even handlers
            virtual void on_playback_updated(const PlaybackState &state);
            virtual void on_playback_sync_finished(const std::string &err_msg);
            virtual void on_devices_changed(const DevicesList &devices);

            // helpers
            bool check_text_label(int dialog_item_id, const std::wstring &text_to_check) const;
            intptr_t set_control_text(int control_id, const std::wstring &text);
            intptr_t set_control_enabled(int control_id, bool is_enabled);

        private:
            HANDLE hdlg;
            bool visible = false;
            bool are_dlg_events_suppressed = true;
            std::vector<FarDialogItem> dlg_items_layout;
            spotify::Api& api;

            typedef bool (PlayerDialog::*ControlHandler)(void*);
            static const std::map<DialogControls, std::map<FARMESSAGE, ControlHandler> > dlg_event_handlers;
        };
    }
}

#endif //PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3