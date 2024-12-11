#ifndef PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3
#define PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3

#include "stdafx.h"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        using spotify::DevicesList;
        using spotify::Track;
        
        // TODO: add links to art with redirectino to browser
        class PlayerDialog: public spotify::ApiProtocol
        {
        public:
            enum
            {
                ID_BOX = 0,
                ID_TITLE,
                ID_TRACK_BAR,
                ID_TRACK_TIME,
                ID_TRACK_TOTAL_TIME,
                ID_ARTIST_NAME,
                ID_TRACK_NAME,
                ID_PLAY_BTN,
                ID_PREV_BTN,
                ID_NEXT_BTN,
                ID_LIKE_BTN,
                ID_VOLUME_LABEL,
                ID_REPEAT_BTN,
                ID_SHUFFLE_BTN,
                ID_DEVICES_COMBO,
                TOTAL_ELEMENTS_COUNT,
            }
            DialogControls;

            static const wchar_t TRACK_BAR_CHAR_UNFILLED = 0x2591;
            static const wchar_t TRACK_BAR_CHAR_FILLED = 0x2588;

            static const int width = 60, height = 10;
            static const int view_x = 2, view_y = 2, view_width = width - 2, view_height = height - 2;
            static const int view_center_x = (view_width + view_x)/2, view_center_y = (view_height + view_y)/2;

        public:
            PlayerDialog(spotify::Api& api);
            virtual ~PlayerDialog();

            bool show();
            bool hide(bool is_silent = false);

            bool is_visible() const { return visible; }

        protected:
	        intptr_t Dlg_OnCtlColorDlgItem(HANDLE hdlg, intptr_t id, void* par2);
	        friend intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2);

            bool check_text_label(int dialog_item_id, const std::wstring& text_to_check) const;
            intptr_t set_control_text(int control_id, const std::wstring& text);
            intptr_t set_control_enabled(int control_id, bool is_enabled);

            void update_track_bar(int track_total_time = 0, int track_played_time = 0);
            void update_controls_block(const spotify::PlaybackState& state);
            void update_track_info(const std::string& artist_name = "", const std::string& track_name = "");
            void update_devices_list(const DevicesList& devices);

            bool on_skip_to_next_btn_click();
            bool on_skip_to_previous_btn_click();
            virtual void on_playback_updated(const spotify::PlaybackState& state);
            virtual void on_playback_sync_failed(const std::string& err_msg);
            virtual void on_devices_changed(const DevicesList& devices);

        private:
            HANDLE hdlg;
            bool visible = false;
            std::vector<FarDialogItem> dlg_items_layout;
            spotify::Api& api;
            int volume_percent = 100;
        };
    }
}

#endif //PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3