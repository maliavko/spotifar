#ifndef PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3
#define PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3

#include "stdafx.h"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        class PlayerDialog: public spotify::IApiObserver
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

            static const int WIDTH = 60, HEIGHT = 10;

        public:
            PlayerDialog(spotify::Api& api);
            virtual ~PlayerDialog();

            bool show();
            bool hide();

            bool is_visible() const { return visible; }

        protected:
	        intptr_t Dlg_OnCtlColorDlgItem(HANDLE hdlg, intptr_t id, void* par2);
	        friend intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2);

            bool check_text_label(int dialog_item_id, const std::wstring& text_to_check) const;

            virtual void on_track_progress_changed() {};

        private:
            HANDLE hdlg;
            bool visible = false;
            std::vector<FarDialogItem> dlg_items_layout;
            spotify::Api& api;
        };
    }
}

#endif //PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3