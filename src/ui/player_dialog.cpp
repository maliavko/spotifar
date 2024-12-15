#include "ui/player_dialog.hpp"
#include "config.hpp"
#include "lng.hpp"

namespace spotifar
{
    namespace ui
    {
        using config::get_msg;
        using config::send_dlg_msg;
        using utils::NoRedraw;
        
        // a helper class to suppress dlg_proc function from handling incoming events,
        // while the instance of the class exists in the particular scope
        struct [[nodiscard]] DlgEventsSuppressor
        {
            bool were_events_suppressed;
            PlayerDialog& dialog;

            DlgEventsSuppressor(PlayerDialog& d):
                dialog(d)
            {
                were_events_suppressed = dialog.are_dlg_events_suppressed;
                dialog.are_dlg_events_suppressed = true;
            }

            ~DlgEventsSuppressor()
            {
                dialog.are_dlg_events_suppressed = were_events_suppressed;
            }
        };

        const std::map<PlayerDialog::DialogControls, std::map<FARMESSAGE, PlayerDialog::ControlHandler> > PlayerDialog::dlg_event_handlers{
            { PlayerDialog::NO_CONTROL, {
                { DN_CONTROLINPUT, &PlayerDialog::on_input_received },
            }},
            { PlayerDialog::DEVICES_COMBO, {
                { DN_EDITCHANGE, &PlayerDialog::on_devices_item_selected },
            }},
            { PlayerDialog::NEXT_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_skip_to_next_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_playback_control_style_applied },
            }},
            { PlayerDialog::PREV_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_skip_to_previous_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_playback_control_style_applied },
            }},
            { PlayerDialog::PLAY_BTN, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_playback_control_style_applied },
            }},
            { PlayerDialog::TRACK_BAR, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_track_bar_style_applied },
            }},
            { PlayerDialog::ARTIST_NAME, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_inactive_control_style_applied },
            }},
            { PlayerDialog::LIKE_BTN, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_inactive_control_style_applied },
            }},
            { PlayerDialog::SHUFFLE_BTN, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_inactive_control_style_applied },
            }},
        };

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
                { DI_BUTTON,        view_center_x - 15, view_height, 1, 1,      0, nullptr,nullptr,     DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE | DIF_RIGHTTEXT, get_msg(MPlayerShuffleBtn) },
                
                // devices box
                { DI_COMBOBOX,    view_width-13, 1, view_width-1, 0,            {}, nullptr, nullptr,   DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND | DIF_DROPDOWNLIST | DIF_NOFOCUS, L"" },
            });
        }

        PlayerDialog::~PlayerDialog()
        {
            hide();
        }
	
        bool PlayerDialog::handle_dlg_proc_event(intptr_t msg_id, DialogControls control_id, void* param)
        {
            if (are_dlg_events_suppressed)
                return false;
            
            // first, trying to find a handler among the given control id event handers;
            // in negative scenario, trying to search for a hander among global handlers @NO_CONTROL id;
            // otherwise @false return control to the @dlg_proc function
            for (auto ctrl_id: { control_id, NO_CONTROL })
            {
                auto it = dlg_event_handlers.find(ctrl_id);
                if (it != dlg_event_handlers.end())
                {
                    for (auto& [mid, handler]: it->second)
                        if (mid == msg_id)
                            return (this->*handler)(param);
                }
            }
            return false;
        }

        intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2)
        {
            static PlayerDialog* dialog = nullptr;
            if (msg == DN_INITDIALOG)
            {
                dialog = reinterpret_cast<PlayerDialog*>(param2);
                return FALSE;
            }
            else if (msg == DN_CLOSE)
            {
                // the event comes from far and ui will be closed automatically,
                // to avoid recursion, we're telling "hide" not to fire a message
                dialog->hide(false);
                dialog = nullptr;
                return TRUE;
            }

            if (dialog && dialog->handle_dlg_proc_event(msg, (PlayerDialog::DialogControls)param1, param2))
                return TRUE;

            return config::PsInfo.DefDlgProc(hdlg, msg, param1, param2);
        }

        bool PlayerDialog::show()
        {
            if (!visible)
            {
                hdlg = config::PsInfo.DialogInit(&MainGuid, &PlayerDialogGuid, -1, -1, width, height, 0,
                    &dlg_items_layout[0], std::size(dlg_items_layout), 0, FDLG_SMALLDIALOG | FDLG_NONMODAL, &dlg_proc, this);
                are_dlg_events_suppressed = false;
                
                api.start_listening(this);

                if (hdlg != NULL)
                {
                    visible = true;
                    
                    update_devices_list(api.get_available_devices());

                    return true;
                }
            }
            return false;
        }

        bool PlayerDialog::hide(bool close_ui)
        {
            if (visible)
            {
                api.stop_listening(this);

                if (hdlg != NULL && close_ui)
                    send_dlg_msg(hdlg, DM_CLOSE, -1, 0);
                
                hdlg = NULL;
                visible = false;
                are_dlg_events_suppressed = true;

                return true;
            }
            return false;
        }
        
        void PlayerDialog::update_devices_list(const DevicesList& devices)
        {
	        NoRedraw nr(hdlg);
            DlgEventsSuppressor s(*this);

            send_dlg_msg(this->hdlg, DM_LISTDELETE, DEVICES_COMBO, NULL);

            for (int i = 0; i < devices.size(); i++)
            {
                auto& dev = devices[i];

                FarListItem item{ LIF_NONE, dev.user_name.c_str(), NULL, NULL };
                if (dev.is_active)
                    item.Flags |= LIF_SELECTED;
                    
                FarList list{ sizeof(FarList), 1, &item };
                send_dlg_msg(this->hdlg, DM_LISTADD, DEVICES_COMBO, &list);
                
                FarListItemData data{sizeof(FarListItemData), i, dev.id.size(), (void*)dev.id.c_str()};
                send_dlg_msg(this->hdlg, DM_LISTSETDATA, DEVICES_COMBO, &data);
            }
        }
        
        void PlayerDialog::update_track_info(const std::string& artist_name, const std::string& track_name)
        {
	        NoRedraw nr(hdlg);

            static std::wstring artist_user_name, track_user_name;

            artist_user_name = utils::to_wstring(artist_name);
            track_user_name = utils::to_wstring(track_name);
            
            set_control_text(ARTIST_NAME, artist_user_name);
            set_control_text(TRACK_NAME, track_user_name);
        }
        
        void PlayerDialog::update_controls_block(const PlaybackState& state)
        {
	        NoRedraw nr(hdlg);

            static std::wstring volume_label, repeat_label, shuffle_label;

            // TODO: complete permissions, they come always "false"

            // play, next, prev
            //set_control_enabled(PLAY_BTN, state.permissions.resuming);
            //set_control_enabled(NEXT_BTN, state.permissions.skipping_next);
            //set_control_enabled(PLAY_BTN, state.permissions.skipping_prev);

            // update shuffle button
            shuffle_label = utils::to_wstring(state.shuffle_state ? "Shuffle" : "No shuffle");
            set_control_text(SHUFFLE_BTN, shuffle_label);
            //set_control_enabled(PLAY_BTN, state.permissions.toggling_shuffle);

            // update repeat button
            repeat_label = utils::to_wstring(state.repeat_state);
            set_control_text(REPEAT_BTN, repeat_label);

            // update volume
            volume_label = std::format(L"[{:3}%]", state.device.volume_percent);
            set_control_text(VOLUME_LABEL, volume_label);
            //set_control_enabled(VOLUME_LABEL, state.device.supports_volume);
        }
        
        // if @track_total_time is 0, the trackback will be filled empty
        void PlayerDialog::update_track_bar(int track_total_time, int track_played_time)
        {
	        NoRedraw nr(hdlg);

            // TODO: time ticking is stuttering, it is needed to try to implement it on the client side
            static std::wstring track_bar, track_time_str, track_total_time_str;

            track_bar = std::wstring(view_width - 14, TRACK_BAR_CHAR_UNFILLED);
            track_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(track_played_time));
            track_total_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(track_total_time));

            if (track_total_time)
            {
                float progress_percent = (float)track_played_time / track_total_time;
                int progress_chars_length = (int)(track_bar.size() * progress_percent);
                fill(track_bar.begin(), track_bar.begin() + progress_chars_length, TRACK_BAR_CHAR_FILLED);
            }
            
            set_control_text(TRACK_BAR, track_bar);
            set_control_text(TRACK_TIME, track_time_str);
            set_control_text(TRACK_TOTAL_TIME, track_total_time_str);
        }

        bool PlayerDialog::on_devices_item_selected(void* dialog_item)
        {
            FarDialogItem* item = reinterpret_cast<FarDialogItem*>(dialog_item);

            size_t pos = send_dlg_msg(hdlg, DM_LISTGETCURPOS, DEVICES_COMBO, NULL);
            auto item_data = send_dlg_msg(hdlg, DM_LISTGETDATA, DEVICES_COMBO, (void*)pos);
            size_t item_data_size = send_dlg_msg(hdlg, DM_LISTGETDATASIZE, DEVICES_COMBO, (void*)pos);
            if (item_data)
            {
                auto device_id = std::string(reinterpret_cast<const char*>(item_data), item_data_size);
                api.transfer_playback(device_id, true);
            }

            //update_devices_list(api.get_available_devices());

            return true;
        }

        bool PlayerDialog::on_input_received(void* input_record)
        {
            INPUT_RECORD* ir = reinterpret_cast<INPUT_RECORD*>(input_record);
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
                                // int volume_to_request = dialog->volume_percent += key == VK_UP ? +5 : -5;
                                // volume_to_request = max(min(volume_to_request, 100), 0);
                                // dialog->api.set_playback_volume(volume_to_request);
                                int i = 0;
                                return true;
                            }
                        }
                    }
                    break;
            }
            return false;
        }
        
        bool PlayerDialog::on_playback_control_style_applied(void* dialog_item_colors)
        {
            FarDialogItemColors* dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            dic->Colors->BackgroundColor = utils::CLR_DGRAY;
            dic->Colors->ForegroundColor = utils::CLR_BLACK;
            return true;
        }
        
        bool PlayerDialog::on_track_bar_style_applied(void* dialog_item_colors)
        {
            FarDialogItemColors* dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            dic->Colors->ForegroundColor = utils::CLR_BLACK;
            return true;
        }
        
        bool PlayerDialog::on_inactive_control_style_applied(void* dialog_item_colors)
        {
            FarDialogItemColors* dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            dic->Colors->ForegroundColor = utils::CLR_DGRAY;
            return true;
        }

        bool PlayerDialog::on_skip_to_next_btn_click(void* empty)
        {
            api.skip_to_next();
            return true;
        }

        bool PlayerDialog::on_skip_to_previous_btn_click(void* empty)
        {
            api.skip_to_previous();
            return true;
        }
        
        void PlayerDialog::on_playback_updated(const PlaybackState& state)
        {
            if (!state.is_empty())
            {
                update_track_info(state.track->artists[0].name, state.track->name);
                update_track_bar(state.track->duration_ms / 1000, state.progress_ms / 1000);
            }
            else
            {
                update_track_info();
                update_track_bar();
            }

            update_controls_block(state);
        }
        
        void PlayerDialog::on_playback_sync_finished(const std::string& exit_msg)
        {
            if (!exit_msg.empty())
                utils::show_far_error_dlg(MFarMessageErrorPlaybackSync, exit_msg);
            
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
                send_dlg_msg(hdlg, DM_GETTEXT, dialog_item_id, &data);
            return text_to_check == data.PtrData;
        }
        
        intptr_t PlayerDialog::set_control_text(int control_id, const std::wstring& text)
        {
            return send_dlg_msg(hdlg, DM_SETTEXTPTR, control_id, (void*)text.c_str());
        }
        
        intptr_t PlayerDialog::set_control_enabled(int control_id, bool is_enabled)
        {
            return send_dlg_msg(hdlg, DM_ENABLE, control_id, (void*)is_enabled);
        }
    }
}