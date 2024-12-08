#include "ui/player_dialog.hpp"
#include "config.hpp"
#include "lng.hpp"

namespace spotifar
{
    namespace ui
    {
        using config::get_msg;

        // TODO: save sync data to compare it with the upcoming one and update only the controls changed
        PlayerDialog::PlayerDialog(spotify::Api& api):
            api(api)
        {
            static wchar_t player_title[MAX_PATH];
            config::FSF.sprintf(player_title, L" %s ", get_msg(MPluginUserName));

            dlg_items_layout.assign({
                // border
                { DI_DOUBLEBOX,     0, 0, width, height,                                                DIF_NONE, L"" },  // ID_BOX
                { DI_TEXT,          view_center_x - 5, 0, 10, 1,                0, nullptr,nullptr,     DIF_CENTERTEXT, player_title }, // ID_TITLE

                // trackbar
                { DI_TEXT,          view_x, view_height - 2, view_width, 1,     0, nullptr,nullptr,     DIF_CENTERTEXT, L"" },  // ID_TRACK_BAR
                { DI_TEXT,          view_x, view_height - 2, 6, 1,              0, nullptr,nullptr,     DIF_LEFTTEXT, L"00:00" },  // ID_TRACK_TIME
                { DI_TEXT,          view_width - 5, view_height - 2, 6, 1,      0, nullptr,nullptr,     DIF_RIGHTTEXT, L"00:00" },  // ID_TRACK_TOTAL_TIME
                
                // playing info
                { DI_TEXT,          view_x, view_height - 5, view_width, 1,     0, nullptr,nullptr,     DIF_LEFTTEXT, L"" },  // ID_ARTIST_NAME
                { DI_TEXT,          view_x, view_height - 4, view_width, 1,     0, nullptr,nullptr,     DIF_LEFTTEXT, L"" },  // ID_TRACK_NAME

                // controls
                { DI_BUTTON,        view_center_x - 2, view_height, 1, 1,       0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, get_msg(MPlayerPlayBtn) },
                { DI_BUTTON,        view_center_x - 7, view_height, 1, 1,       0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, get_msg(MPlayerPrevBtn) },
                { DI_BUTTON,        view_center_x + 4, view_height, 1, 1,       0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, get_msg(MPlayerNextBtn) },
                { DI_BUTTON,        view_x, view_height, 1, 1,                  0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, get_msg(MPlayerLikeBtn) },
                { DI_TEXT,          view_width - 6, view_height, 1, 1,          0, nullptr,nullptr,     DIF_CENTERTEXT | DIF_NOFOCUS | DIF_BTNNOCLOSE, L"[---%]" },
                { DI_BUTTON,        view_center_x + 9, view_height, 1, 1,       0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, get_msg(MPlayerRepeatNoneBtn) },
                { DI_BUTTON,        view_center_x - 15, view_height, 1, 1,      0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE, get_msg(MPlayerShuffleBtn) },
                
                // devices box
                { DI_COMBOBOX,    view_width-13, 1, view_width-1, 0,            {}, nullptr, nullptr,   DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND | DIF_DROPDOWNLIST | DIF_NOFOCUS, L"" },
            });

            update_track_bar();
        }

        PlayerDialog::~PlayerDialog()
        {
            hide();
        }

        // TODO: rename and reconsider function's content
        intptr_t PlayerDialog::Dlg_OnCtlColorDlgItem(HANDLE hdlg, intptr_t id, void* par2)
        {
            WORD loattr = 0, hiattr = 0;
            intptr_t res;
            FarDialogItemColors* fdic = (FarDialogItemColors*)par2;
            res = 0;

            switch (id)
            {
                case ID_PLAY_BTN:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->BackgroundColor = utils::CLR_DGRAY;
                    fdic->Colors->ForegroundColor = utils::CLR_BLACK;
                    res = TRUE;
                    break;
                case ID_ARTIST_NAME:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->ForegroundColor = utils::CLR_DGRAY;
                    break;
                case ID_PREV_BTN:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->BackgroundColor = utils::CLR_DGRAY;
                    fdic->Colors->ForegroundColor = utils::CLR_BLACK;
                    break;
                case ID_NEXT_BTN:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->BackgroundColor = utils::CLR_DGRAY;
                    fdic->Colors->ForegroundColor = utils::CLR_BLACK;
                    break;
                case ID_LIKE_BTN:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->ForegroundColor = utils::CLR_DGRAY;
                    break;
                case ID_SHUFFLE_BTN:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->ForegroundColor = utils::CLR_DGRAY;
                    break;
                case ID_DEVICES_COMBO:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->ForegroundColor = utils::CLR_BLACK;
                    fdic->Colors->BackgroundColor = utils::CLR_DGRAY;
                    break;
                case ID_VOLUME_LABEL:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    break;
                case ID_TRACK_BAR:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->ForegroundColor = utils::CLR_BLACK;
                    break;
                case ID_TRACK_TOTAL_TIME:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    break;
                case ID_TRACK_TIME:
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    break;
                case ID_REPEAT_BTN:
                {
                    int color = utils::CLR_DGRAY;
                    FarDialogItemData data = { sizeof(FarDialogItemData), 0, new wchar_t[32] };
                    auto i = config::PsInfo.SendDlgMessage(hdlg, DM_GETTEXT, ID_REPEAT_BTN, &data);
                    
                    if (lstrcmp(data.PtrData, L"Repeat"))
                    {
                        color = utils::CLR_BLACK;
                    }
                    fdic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
                    fdic->Colors->ForegroundColor = color;
                    break;
                }
                default:
                    res = FALSE;
                    break;
            }

            return res;
        }
	
        intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2)
        {
            intptr_t id = param1;
            intptr_t res = 0;
            INPUT_RECORD* ir;
            static PlayerDialog* dialog = nullptr;

            switch (msg)
            {
                case DN_INITDIALOG:
                    dialog = reinterpret_cast<PlayerDialog*>(param2);
                    res = FALSE; // no changes made
                    break;
                case DN_CLOSE:
                    dialog->hide();
                    dialog = NULL;
                    res = TRUE; // dialog can be closed
                    break;
                case DN_BTNCLICK:
                    break;
                case DN_CTLCOLORDLGITEM:
                    res = dialog->Dlg_OnCtlColorDlgItem(hdlg, param1, param2);
                    break;
                case DN_CONTROLINPUT:
                {
                    ir = (INPUT_RECORD*)param2;
                    switch (ir->EventType)
                    {
                        case KEY_EVENT:
                            if (ir->Event.KeyEvent.bKeyDown)
                            {
                                int key = utils::input_record_to_combined_key(ir->Event.KeyEvent);
                                switch (key)
                                {
                                    case VK_UP:
                                    case VK_DOWN:
                                    {
                                        //std::wstring volume_label = std::format(L"[{:3}%]", dialog->volume);
                                        //config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, PlayerDialog::ID_VOLUME_LABEL, (void*)volume_label.c_str());
                                        break;
                                    }
                                }
                            }
                            break;
                        case MOUSE_EVENT:
                        {
                            if (param1 == PlayerDialog::ID_REPEAT_BTN)
                            {
                                // TODO: consider some logic with cycled iterator
                                if (dialog->check_text_label(PlayerDialog::ID_REPEAT_BTN, L"Repeat"))
                                {
                                    config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, PlayerDialog::ID_REPEAT_BTN,
                                        (void*)get_msg(MPlayerRepeatOneBtn));
                                }
                                else if (dialog->check_text_label(PlayerDialog::ID_REPEAT_BTN, L"Track"))
                                {
                                    config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, PlayerDialog::ID_REPEAT_BTN,
                                        (void*)get_msg(MPlayerRepeatAllBtn));
                                }
                                else if (dialog->check_text_label(PlayerDialog::ID_REPEAT_BTN, L"All"))
                                {
                                    config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, PlayerDialog::ID_REPEAT_BTN,
                                        (void*)get_msg(MPlayerRepeatNoneBtn));
                                }
                                res = true;

                            }
                            break;
                        }
                    }
                    break;
                }
                default:
                    res = config::PsInfo.DefDlgProc(hdlg, msg, param1, param2);
            }
            return res;
        }

        bool PlayerDialog::show()
        {
            if (visible)
                return true;
            
            hdlg = config::PsInfo.DialogInit(&MainGuid, &PlayerDialogGuid, -1, -1, width, height, 0,
                &dlg_items_layout[0], std::size(dlg_items_layout), 0, FDLG_SMALLDIALOG | FDLG_NONMODAL, &dlg_proc, this);
            
            api.start_listening(this);

            if (hdlg != NULL)
            {
                visible = true;
                return true;
            }

            return false;
        }

        bool PlayerDialog::hide()
        {
            if (!visible)
                return true;

            api.stop_listening(this);

            if (hdlg != NULL)
            {
                config::PsInfo.SendDlgMessage(hdlg, DM_CLOSE, -1, 0);
                visible = false;
                hdlg = NULL;
            }

            return TRUE;
        }
        
        void PlayerDialog::update_devices_list(const DevicesList& devices)
        {
            // TODO: not finished, check when there are not devices
            static std::vector<FarListItem> items;
            for (int i = 0; i < devices.size(); i++)
            {
                auto& d = devices[i];

                FarListItem item;
                if (d.is_active)
                    item.Flags |= LIF_SELECTED;

                FarListItemData data{sizeof(FarListItemData), i, 0, (void*)d.id.c_str()};

                items.push_back({
                    0, d.user_name.c_str(), (intptr_t)&data, 0
                });
            }

            FarList list={sizeof(FarList), items.size(), &items[0]};

            config::PsInfo.SendDlgMessage(hdlg, DM_LISTSET, ID_DEVICES_COMBO, &list);
        }
        
        void PlayerDialog::update_track_info()
        {
                // ID_ARTIST_NAME,
                // ID_TRACK_NAME,
        }
        
        void PlayerDialog::update_controls_block()
        {                
                // ID_PLAY_BTN,
                // ID_PREV_BTN,
                // ID_NEXT_BTN,
                // ID_LIKE_BTN,
                // ID_VOLUME_LABEL,
                // ID_REPEAT_BTN,
                // ID_SHUFFLE_BTN,
        }
        
        // if @track_total_time is 0, the trackback will be filled empty
        void PlayerDialog::update_track_bar(int track_total_time, int track_played_time)
        {
            static std::wstring track_bar(view_width - 14, TRACK_BAR_CHAR_UNFILLED);
            static std::wstring track_time_str(std::format(L"{:%M:%S}", std::chrono::seconds(track_played_time)));
            static std::wstring track_total_time_str(std::format(L"{:%M:%S}", std::chrono::seconds(track_total_time)));

            if (track_total_time)
            {
                float progress_percent = (float)track_played_time / track_total_time;
                int progress_chars_length = (int)(track_bar.size() * progress_percent);
                fill(track_bar.begin(), track_bar.begin() + progress_chars_length, TRACK_BAR_CHAR_FILLED);
            }
            
            config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_TRACK_BAR, (void*)track_bar.c_str());
            config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_TRACK_TIME, (void*)track_time_str.c_str());
            config::PsInfo.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_TRACK_TOTAL_TIME, (void*)track_total_time_str.c_str());
        }
        
        void PlayerDialog::on_playback_updated(const spotify::PlaybackState& state)
        {
	        config::PsInfo.SendDlgMessage(hdlg, DM_ENABLEREDRAW, FALSE, 0);

            if (state.is_empty())
            {
                update_track_bar();
            }
            
	        config::PsInfo.SendDlgMessage(hdlg, DM_ENABLEREDRAW, TRUE, 0);
        }
        
        void PlayerDialog::on_playback_sync_failed(const std::string& err_msg)
        {
            utils::show_far_error_dlg(MFarMessageErrorPlaybackSync, err_msg);
            hide();
        }
        
        void PlayerDialog::on_devices_changed(const DevicesList& devices)
        {
            update_devices_list(devices);
        }
        
        bool PlayerDialog::check_text_label(int dialog_item_id, const std::wstring& text_to_check) const
        {
            static wchar_t ptrdata[32];
            FarDialogItemData data = { sizeof(FarDialogItemData), 0, ptrdata };
                config::PsInfo.SendDlgMessage(hdlg, DM_GETTEXT, dialog_item_id, &data);
            return text_to_check == data.PtrData;
        }
    }
}