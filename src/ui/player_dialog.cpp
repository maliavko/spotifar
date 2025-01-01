#include "ui/player_dialog.hpp"

namespace spotifar
{
    namespace ui
    {
        namespace far3 = utils::far3;
        using far3::get_msg;
        using far3::send_dlg_msg;
        using far3::NoRedraw;
        using far3::push_synchro_task;
        using namespace std::literals;
        
        static const wchar_t TRACK_BAR_CHAR_UNFILLED = 0x2591;
        static const wchar_t TRACK_BAR_CHAR_FILLED = 0x2588;

        static const wchar_t *PLAY_BTN_LABEL = L"[ \x25ba ]";
        static const wchar_t *PAUSE_BTN_LABEL = L"[ ‖ ]";
        static const wchar_t *NEXT_BTN_LABEL = L"[>>]";
        static const wchar_t *PREV_BTN_LABEL = L"[<<]";
        static const wchar_t *LIKE_BTN_LABEL = L"[+]";

        static const unsigned int SEEKING_STEP = 3, VOLUME_STEP = 1;

        static const int width = 60, height = 10;
        static const int view_x = 2, view_y = 2, view_width = width - 2, view_height = height - 2;
        static const int view_center_x = (view_width + view_x)/2, view_center_y = (view_height + view_y)/2;
        
        enum DialogControls
        {
            NO_CONTROL = -1,
            BOX,
            TITLE,
            TRACK_BAR,
            TRACK_TIME,
            TRACK_TOTAL_TIME,
            SOURCE_NAME,
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
        
        // a helper class to suppress dlg_proc function from handling incoming events,
        // while the instance of the class exists in the particular scope
        struct _NODISCARD DlgEventsSuppressor
        {
            bool were_events_suppressed;
            PlayerDialog& dialog;

            DlgEventsSuppressor(PlayerDialog &d):
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

        FarDialogItem control(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
                              FARDIALOGITEMFLAGS flags, const wchar_t *data = L"")
        {
            return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
        }

        auto btn_flags = DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE;
        auto combo_flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE |
                           DIF_DROPDOWNLIST;
    
        std::vector<FarDialogItem> dlg_items_layout{
            // border
            control(DI_DOUBLEBOX,   0, 0, width, height,                            DIF_NONE), // BOX
            control(DI_TEXT,        view_center_x - 5, 0, 10, 1,                    DIF_CENTERTEXT), // TITLE

            // trackbar
            control(DI_TEXT,        view_x + 6, view_height - 2, view_width - 6, 1, DIF_CENTERTEXT), // TRACK_BAR
            control(DI_TEXT,        view_x, view_height - 2, 6, 1,                  DIF_LEFTTEXT), // TRACK_TIME
            control(DI_TEXT,        view_width - 5, view_height - 2, 6, 1,          DIF_RIGHTTEXT), // TRACK_TOTAL_TIME
            
            // playing info
            control(DI_TEXT,        view_x, 1, view_width, 1,                       DIF_LEFTTEXT), // SOURCE_NAME
            control(DI_TEXT,        view_x, view_height - 5, view_width, 1,         DIF_LEFTTEXT), // ARTIST_NAME
            control(DI_TEXT,        view_x, view_height - 4, view_width, 1,         DIF_LEFTTEXT), // TRACK_NAME

            // controls
            control(DI_BUTTON,      view_center_x - 2, view_height, 1, 1,           btn_flags, PLAY_BTN_LABEL),
            control(DI_BUTTON,      view_center_x - 7, view_height, 1, 1,           btn_flags, PREV_BTN_LABEL),
            control(DI_BUTTON,      view_center_x + 4, view_height, 1, 1,           btn_flags, NEXT_BTN_LABEL),
            control(DI_BUTTON,      view_x, view_height, 1, 1,                      btn_flags, LIKE_BTN_LABEL),
            control(DI_TEXT,        view_width - 6, view_height, 1, 1,              btn_flags | DIF_RIGHTTEXT, L"[---%]"),
            control(DI_BUTTON,      view_center_x + 9, view_height, 1, 1,           btn_flags),  // REPEAT
            control(DI_BUTTON,      view_center_x - 15, view_height, 1, 1,          btn_flags),  // SHUFFLE
            
            // devices box
            control(DI_COMBOBOX,    view_width - 13, 1, view_width - 1, 0,          combo_flags),
        };

        typedef bool (PlayerDialog::*ControlHandler)(void*);
        const std::map<DialogControls, std::map<FARMESSAGE, ControlHandler>> dlg_event_handlers{
            { NO_CONTROL, {
                { DN_CONTROLINPUT, &PlayerDialog::on_input_received },
            }},
            { DEVICES_COMBO, {
                { DN_EDITCHANGE, &PlayerDialog::on_devices_item_selected },
            }},
            { NEXT_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_skip_to_next_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_playback_control_style_applied },
            }},
            { PREV_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_skip_to_previous_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_playback_control_style_applied },
            }},
            { PLAY_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_play_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_playback_control_style_applied },
            }},
            { TRACK_BAR, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_track_bar_style_applied },
                { DN_CONTROLINPUT, &PlayerDialog::on_track_bar_input_received },
            }},
            { ARTIST_NAME, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_inactive_control_style_applied },
            }},
            { SOURCE_NAME, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_inactive_control_style_applied },
            }},
            { LIKE_BTN, {
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_inactive_control_style_applied },
            }},
            { SHUFFLE_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_shuffle_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_shuffle_btn_style_applied },
            }},
            { REPEAT_BTN, {
                { DN_BTNCLICK, &PlayerDialog::on_repeat_btn_click },
                { DN_CTLCOLORDLGITEM, &PlayerDialog::on_repeat_btn_style_applied },
            }},
        };

        PlayerDialog::PlayerDialog(Api &api):
            api(api),
            volume(0, 100, 1),
            track_progress(0, 0, 5),
            shuffle_state({ true, false }),
            repeat_state({ PlaybackState::REPEAT_OFF, PlaybackState::REPEAT_TRACK,
                PlaybackState::REPEAT_CONTEXT })
        {
        }

        PlayerDialog::~PlayerDialog()
        {
            hide();
        }
	
        bool PlayerDialog::handle_dlg_proc_event(intptr_t msg_id, intptr_t control_id, void *param)
        {
            if (are_dlg_events_suppressed)
                return false;

            // first, trying to find a handler among the given control id event handers;
            // in negative scenario, trying to search for a hander among global handlers @NO_CONTROL id;
            // otherwise @false return control to the @dlg_proc function
            for (auto ctrl: { DialogControls(control_id), NO_CONTROL })
            {
                auto it = dlg_event_handlers.find(ctrl);
                if (it != dlg_event_handlers.end())
                {
                    for (auto& [mid, handler]: it->second)
                        if (mid == msg_id)
                            return (this->*handler)(param);
                }
            }
            return false;
        }

        intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
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

            if (dialog && dialog->handle_dlg_proc_event(msg, param1, param2))
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
                
                api.start_listening(dynamic_cast<BasicApiObserver*>(this));
                api.start_listening(dynamic_cast<PlaybackObserver*>(this));

                if (hdlg != NULL)
                {
                    visible = true;
                    
                    static std::wstring title(std::format(L" {} ", get_msg(MPluginUserName)));
                    set_control_text(TITLE, title);

                    // TODO: all the updates are coming from sync-thread, this one is in the main one,
                    // which can create memory racing situation. Reconsider the logic here
                    // initial ui initialization with the cached data
                    auto &state = api.get_playback_state();
                    on_track_changed(state.item);
                    on_track_progress_changed(state.item.duration, state.progress);
                    on_volume_changed(state.device.volume_percent);
                    on_state_changed(state.is_playing);
                    on_context_changed(state.context);
                    on_shuffle_state_changed(state.shuffle_state);
                    on_repeat_state_changed(state.repeat_state);
                    
                    on_devices_changed(api.get_available_devices());

                    return true;
                }
            }
            return false;
        }

        bool PlayerDialog::hide(bool close_ui)
        {
            if (visible)
            {
                api.stop_listening(dynamic_cast<BasicApiObserver*>(this));
                api.stop_listening(dynamic_cast<PlaybackObserver*>(this));

                if (hdlg != NULL && close_ui)
                    send_dlg_msg(hdlg, DM_CLOSE, -1, 0);
                
                hdlg = NULL;
                visible = false;
                are_dlg_events_suppressed = true;

                return true;
            }
            return false;
        }

        bool PlayerDialog::on_devices_item_selected(void *dialog_item)
        {
            FarDialogItem* item = reinterpret_cast<FarDialogItem*>(dialog_item);

            size_t pos = send_dlg_msg(hdlg, DM_LISTGETCURPOS, DEVICES_COMBO, NULL);
            auto item_data = send_dlg_msg(hdlg, DM_LISTGETDATA, DEVICES_COMBO, (void*)pos);
            size_t item_data_size = send_dlg_msg(hdlg, DM_LISTGETDATASIZE, DEVICES_COMBO, (void*)pos);
            if (item_data)
            {
                auto &state = api.get_playback_state();
                api.transfer_playback(
                    std::string(reinterpret_cast<const char*>(item_data), item_data_size),
                    state.is_playing
                );
            }
            return true;
        }

        bool PlayerDialog::on_input_received(void *input_record)
        {
            auto state = api.get_playback_state();
            INPUT_RECORD *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
            switch (ir->EventType)
            {
                case KEY_EVENT:
                    if (ir->Event.KeyEvent.bKeyDown)
                    {
                        int key = far3::input_record_to_combined_key(ir->Event.KeyEvent);
                        switch (key)
                        {
                            case VK_SPACE:
                                on_play_btn_click(nullptr);
                                return true;

                            case VK_RIGHT + far3::KEY_ALT:
                                on_skip_to_next_btn_click(nullptr);
                                return true;

                            case VK_LEFT + far3::KEY_ALT:
                                on_skip_to_previous_btn_click(nullptr);
                                return true;

                            case VK_LEFT:
                            case VK_RIGHT:
                            {
                                update_track_bar(track_progress.get_higher_boundary(),
                                                 key == VK_RIGHT ? track_progress.next() : track_progress.prev());
                                return true;
                            }
                            case VK_UP:
                            case VK_DOWN:
                            {
                                update_volume_bar(key == VK_UP ? volume.next() : volume.prev());
                                return true;
                            }
                            case far3::KEY_R:
                            {
                                update_repeat_btn(repeat_state.next());
                                return true;
                            }
                            case far3::KEY_S:
                            {
                                update_shuffle_btn(shuffle_state.next());
                                return true;
                            }
                            case far3::KEY_D + far3::KEY_ALT:
                            {
                                send_dlg_msg(hdlg, DM_SETDROPDOWNOPENED, DEVICES_COMBO, (void*)TRUE);
                                return true;
                            }
                        }
                    }
                    break;
            }
            return false;
        }
        
        bool PlayerDialog::on_playback_control_style_applied(void *dialog_item_colors)
        {
            FarDialogItemColors* dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            dic->Colors->BackgroundColor = far3::CLR_DGRAY;
            dic->Colors->ForegroundColor = far3::CLR_BLACK;
            return true;
        }
        
        bool PlayerDialog::on_track_bar_style_applied(void *dialog_item_colors)
        {
            FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            dic->Colors->ForegroundColor = far3::CLR_BLACK;
            return true;
        }
        
        bool PlayerDialog::on_track_bar_input_received(void *input_record)
        {
            auto &playback = api.get_playback_state();
            if (playback.is_empty())
                return false;

            INPUT_RECORD *ir = reinterpret_cast<INPUT_RECORD*>(input_record);

            SMALL_RECT dlg_rect;
            far3::send_dlg_msg(hdlg, DM_GETDLGRECT, 0, &dlg_rect);
            
            auto track_bar_layout = dlg_items_layout[TRACK_BAR];
            auto track_bar_length = track_bar_layout.X2 - track_bar_layout.X1;
            auto click_pos = ir->Event.MouseEvent.dwMousePosition.X - (dlg_rect.Left + track_bar_layout.X1);
            auto progress_percent = (float)click_pos / track_bar_length;

            api.seek_to_position((int)(playback.item.duration_ms * progress_percent));
            return true;
        }
        
        bool PlayerDialog::on_inactive_control_style_applied(void *dialog_item_colors)
        {
            FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            dic->Colors->ForegroundColor = far3::CLR_DGRAY;
            return true;
        }
        
        bool PlayerDialog::on_shuffle_btn_style_applied(void *dialog_item_colors)
        {
            FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            if (shuffle_state.get_offset_value())
            {
                dic->Colors->ForegroundColor = far3::CLR_BLACK;
            }
            else
            {
                dic->Colors->ForegroundColor = far3::CLR_DGRAY;
            }
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            return true;
        }
        
        bool PlayerDialog::on_repeat_btn_style_applied(void *dialog_item_colors)
        {
            FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
            if (repeat_state.get_offset_value() != PlaybackState::REPEAT_OFF)
            {
                dic->Colors->ForegroundColor = far3::CLR_BLACK;
            }
            else
            {
                dic->Colors->ForegroundColor = far3::CLR_DGRAY;
            }
            dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
            return true;
        }

        bool PlayerDialog::on_skip_to_next_btn_click(void *empty)
        {
            api.skip_to_next();
            return true;
        }

        bool PlayerDialog::on_skip_to_previous_btn_click(void *empty)
        {
            api.skip_to_previous();
            return true;
        }

        bool PlayerDialog::on_shuffle_btn_click(void *empty)
        {
            update_shuffle_btn(shuffle_state.next());
            return true;
        }

        bool PlayerDialog::on_repeat_btn_click(void *empty)
        {
            update_repeat_btn(repeat_state.next());
            return true;
        }

        bool PlayerDialog::on_play_btn_click(void *empty)
        {
            // TODO: handle errors
            auto &playback = api.get_playback_state();
            if (playback.is_playing)
            {
                api.pause_playback();
                return true;
            }
            else if (!playback.is_empty())
            {
                api.start_playback(playback.context.uri, playback.item.get_uri(),
                    playback.progress_ms, playback.device.id);
                return true;
            }
            return false;
        }
        
        void PlayerDialog::on_playback_sync_finished(const std::string &exit_msg)
        {
            if (!exit_msg.empty())
                far3::show_far_error_dlg(MFarMessageErrorPlaybackSync, exit_msg);
            
            hide();
        }
        
        void PlayerDialog::on_devices_changed(const DevicesList &devices)
        {
	        NoRedraw nr(hdlg);
            DlgEventsSuppressor s(*this);

            send_dlg_msg(this->hdlg, DM_LISTDELETE, DEVICES_COMBO, NULL);

            for (int i = 0; i < devices.size(); i++)
            {
                auto& dev = devices[i];

                FarListItem item{ LIF_NONE, dev.name.c_str(), NULL, NULL };
                if (dev.is_active)
                    item.Flags |= LIF_SELECTED;
                    
                FarList list{ sizeof(FarList), 1, &item };
                send_dlg_msg(this->hdlg, DM_LISTADD, DEVICES_COMBO, &list);
                
                FarListItemData data{sizeof(FarListItemData), i, dev.id.size(), (void*)dev.id.c_str()};
                send_dlg_msg(this->hdlg, DM_LISTSETDATA, DEVICES_COMBO, &data);
            }
        }

        void PlayerDialog::on_track_changed(const Track &track)
        {
	        NoRedraw nr(hdlg);

            static std::wstring track_total_time_str;
            track_total_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(track.duration));

            auto &state = api.get_playback_state();
            track_progress.set_higher_boundary(track.duration);
        
            set_control_text(TRACK_NAME, track.name);
            set_control_text(ARTIST_NAME, track.artists.size() ? track.artists[0].name : L"");
            set_control_text(TRACK_TOTAL_TIME, track_total_time_str);
        }

        void PlayerDialog::update_track_bar(int duration, int progress)
        {
	        NoRedraw nr(hdlg);

            static std::wstring track_bar, track_time_str;

            auto track_bar_layout = dlg_items_layout[TRACK_BAR];
            auto track_bar_size = track_bar_layout.X2 - track_bar_layout.X1;
            track_bar = std::wstring(track_bar_size, TRACK_BAR_CHAR_UNFILLED);
            track_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(progress));

            if (duration)
            {
                float progress_percent = (float)progress / duration;
                int progress_chars_length = (int)(track_bar_size * progress_percent);
                fill(track_bar.begin(), track_bar.begin() + progress_chars_length, TRACK_BAR_CHAR_FILLED);
            }
            
            set_control_text(TRACK_BAR, track_bar);
            set_control_text(TRACK_TIME, track_time_str);
        }

        void PlayerDialog::on_track_progress_changed(int duration, int progress)
        {
            track_progress.set_value(progress);
            
            // prevents from updating, in case we are seeking a new track position,
            // which requires showing a virtual target track bar position
            if (track_progress.is_waiting())
                return;

            return update_track_bar((int)duration, (int)progress);
        }
        
        void PlayerDialog::update_volume_bar(int volume)
        {
	        NoRedraw nr(hdlg);

            static std::wstring volume_label;
            
            volume_label = std::format(L"[{}%]", volume);
            set_control_text(VOLUME_LABEL, volume_label);
        }
        
        void PlayerDialog::on_volume_changed(int vol)
        {
            volume.set_value(vol);

            // prevents from updating, in case we are seeking a new volume value,
            // which requires showing a virtual target vlume bar value
            if (volume.is_waiting())
                return;

            return update_volume_bar(vol);
        }
        
        void PlayerDialog::on_shuffle_state_changed(bool state)
        {
            shuffle_state.set_value(state);

            if (shuffle_state.is_waiting())
                return;

            return update_shuffle_btn(state);
        }
        
        void PlayerDialog::update_shuffle_btn(bool is_shuffling)
        {
	        NoRedraw nr(hdlg);

            static std::wstring shuffle_label;

            shuffle_label = utils::utf8_decode("Shuffle");
            set_control_text(SHUFFLE_BTN, shuffle_label);
        }

        void PlayerDialog::on_repeat_state_changed(const std::string &state)
        {
            repeat_state.set_value(state);

            if (repeat_state.is_waiting())
                return;

            return update_repeat_btn(state);
        }
        
        void PlayerDialog::update_repeat_btn(const std::string &repeate_state)
        {
	        NoRedraw nr(hdlg);

            static std::wstring repeat_label;
            
            // TODO: localize?
            repeat_label = utils::utf8_decode(repeate_state);
            set_control_text(REPEAT_BTN, repeat_label);
        }
        
        void PlayerDialog::on_state_changed(bool is_playing)
        {
            set_control_text(PLAY_BTN, is_playing ? PAUSE_BTN_LABEL : PLAY_BTN_LABEL);
        }
        
        void PlayerDialog::on_context_changed(const Context &ctx)
        {
            std::wstring source_name = L"";
            if (ctx.is_collection())
            {
                source_name = get_msg(MPlayerSourceCollection);
            }
            else if (ctx.is_artist())
            {
                // TODO: implement cache for artists and use it here
                source_name = L"artist";
            }
            else if (ctx.is_album())
            {
                // TODO: implement cache for albums and use it here
                source_name = L"album";
            }
            else if (ctx.is_playlist())
            {
                // TODO: implement cache for playlists and use it here
                source_name = L"playlist";
            }
            
            static std::wstring source_label;
            if (!source_name.empty())
                source_label = std::format(L"{}: {}", get_msg(MPlayerSourceLabel), source_name);
            else
                source_label = L"";

            set_control_text(SOURCE_NAME, source_label);
        }
        
        void PlayerDialog::on_permissions_changed(const Actions &actions)
        {
            // TODO: finish the content
        }
        
        void PlayerDialog::on_sync_thread_tick()
        {
            // here we process delayed controls like seeking position or volume, as they require
            // timer for perform smoothly. But the final operation is executed through the far main
            // thread to avoid threads clashes, plus to process the task through the thread pool

            track_progress.check([this](int p) {
                push_synchro_task([&api = this->api, p]() {
                    auto &state = api.get_playback_state();
                    api.seek_to_position(p * 1000, state.device.id);
                });
            });

            volume.check([this](int v) {
                push_synchro_task([&api = this->api, v]() { api.set_playback_volume(v); });
            });

            shuffle_state.check([this](bool v) {
                push_synchro_task([&api = this->api, v]() { api.toggle_shuffle(v); });
            });

            repeat_state.check([this](const std::string &s) {
                push_synchro_task([&api = this->api, s]() { api.set_repeat_state(s); });
            });
        }
        
        intptr_t PlayerDialog::set_control_text(int control_id, const std::wstring &text)
        {
            return send_dlg_msg(hdlg, DM_SETTEXTPTR, control_id, (void*)text.c_str());
        }
        
        intptr_t PlayerDialog::set_control_enabled(int control_id, bool is_enabled)
        {
            return send_dlg_msg(hdlg, DM_ENABLE, control_id, (void*)is_enabled);
        }
    }
}