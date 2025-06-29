#include "ui/player.hpp"
#include "ui/events.hpp"
#include "ui/dialogs/dialog.hpp"
#include "spotify/interfaces.hpp"
#include "hotkeys_handler.hpp"
#include "lng.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;
namespace keys = utils::keys;
namespace colors = far3::colors;
namespace synchro_tasks = far3::synchro_tasks;

using no_redraw_player = no_redraw<player>;

/// @brief a helper class to suppress `dlg_proc` function from handling incoming events,
/// while the instance of the class exists in the particular scope
struct [[nodiscard]] dlg_events_supressor
{
    dlg_events_supressor(player *d):
        dialog(d)
    {
        were_events_suppressed = dialog->are_dlg_events_suppressed;
        dialog->are_dlg_events_suppressed = true;
    }

    ~dlg_events_supressor()
    {
        dialog->are_dlg_events_suppressed = were_events_suppressed;
    }
    
    bool were_events_suppressed;
    player *dialog;
};

static FarDialogItem control(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
                             FARDIALOGITEMFLAGS flags, const wchar_t *data = L"")
{
    return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
}

enum controls : int
{
    no_control = -1,
    box,
    title,
    track_bar,
    track_time,
    track_total_time,
    source_name,
    artist_name,
    track_name,
    play_btn,
    prev_btn,
    next_btn,
    like_btn,
    volume_label,
    repeat_btn,
    shuffle_btn,
    devices_combo,
};

static const wchar_t
    track_bar_char_unfilled = 0x2591,
    track_bar_char_filled = 0x2588,
    *play_btn_label = L"[ \x25ba ]",
    *pause_btn_label = L"[ ‖ ]",
    *next_btn_label = L"[>>]",
    *prev_btn_label = L"[<<]",
    *like_btn_label = L"[+]";

static const int
    width = 60, height = 11,
    view_x1 = 2, view_y1 = 2, view_x2 = width - 2, view_y2 = height - 2,
    view_center_x = (view_x2 + view_x1)/2, view_center_y = (view_y2 + view_y1)/2;

static const auto
    btn_flags = DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE,
    combo_flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;

static const std::vector<FarDialogItem> dlg_items_layout{
    // border
    control(DI_DOUBLEBOX,   0, 0, width, height-1,                  DIF_NONE), // BOX
    control(DI_TEXT,        view_center_x-5, 0, 10, 1,              DIF_CENTERTEXT), // TITLE

    // trackbar
    control(DI_TEXT,        view_x1+6, view_y2-2, view_x2-6, 1,     DIF_CENTERTEXT), // TRACK_BAR
    control(DI_TEXT,        view_x1, view_y2-2, 6, 1,               DIF_LEFTTEXT), // TRACK_TIME
    control(DI_TEXT,        view_x2-5, view_y2-2, 6, 1,             DIF_RIGHTTEXT), // TRACK_TOTAL_TIME
    
    // playing info
    control(DI_TEXT,        view_x1, 1, view_x2, 1,                 DIF_LEFTTEXT), // SOURCE_NAME
    control(DI_TEXT,        view_x1, view_y2-5, view_x2, 1,         DIF_LEFTTEXT), // ARTIST_NAME
    control(DI_TEXT,        view_x1, view_y2-4, view_x2, 1,         DIF_LEFTTEXT), // TRACK_NAME

    // controls
    control(DI_BUTTON,      view_center_x-2, view_y2, 1, 1,         btn_flags, play_btn_label), // PLAY
    control(DI_BUTTON,      view_center_x-7, view_y2, 1, 1,         btn_flags, prev_btn_label), // PREV_BTN
    control(DI_BUTTON,      view_center_x+4, view_y2, 1, 1,         btn_flags, next_btn_label), // NEXT_BTN
    control(DI_BUTTON,      view_x1, view_y2, 1, 1,                 btn_flags, like_btn_label), // LIKE BTN
    control(DI_TEXT,        view_x2-6, view_y2, 1, 1,               btn_flags | DIF_RIGHTTEXT, L"[---%]"), // VOLUME
    control(DI_BUTTON,      view_center_x+9, view_y2, 1, 1,         btn_flags),  // REPEAT
    control(DI_BUTTON,      view_center_x-15, view_y2, 1, 1,        btn_flags),  // SHUFFLE
    
    // devices box
    control(DI_COMBOBOX,    view_x2-13, 1, view_x2-1, 0,            combo_flags), // DEVICES
};

using control_handler_t = bool (player::*)(void*);

static const std::map<controls, std::map<FARMESSAGE, control_handler_t>> dlg_event_handlers{
    { controls::no_control, {
        { DN_CONTROLINPUT, &player::on_input_received },
    }},
    { controls::devices_combo, {
        { DN_EDITCHANGE, &player::on_devices_item_selected },
    }},
    { controls::next_btn, {
        { DN_BTNCLICK, &player::on_skip_to_next_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_next_btn_style_applied },
    }},
    { controls::prev_btn, {
        { DN_BTNCLICK, &player::on_skip_to_previous_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_prev_btn_style_applied },
    }},
    { controls::play_btn, {
        { DN_BTNCLICK, &player::on_play_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_play_btn_style_applied },
    }},
    { controls::track_bar, {
        { DN_CTLCOLORDLGITEM, &player::on_track_bar_style_applied },
        { DN_CONTROLINPUT, &player::on_track_bar_input_received },
    }},
    { controls::artist_name, {
        { DN_CTLCOLORDLGITEM, &player::on_inactive_control_style_applied },
        { DN_CONTROLINPUT, &player::on_artist_label_input_received },
    }},
    { controls::track_name, {
        { DN_CONTROLINPUT, &player::on_track_label_input_received },
    }},
    { controls::source_name, {
        { DN_CTLCOLORDLGITEM, &player::on_inactive_control_style_applied },
        { DN_CONTROLINPUT, &player::on_source_label_input_received },
    }},
    { controls::like_btn, {
        { DN_CTLCOLORDLGITEM, &player::on_like_btn_style_applied },
        { DN_CONTROLINPUT, &player::on_like_btn_input_received },
    }},
    { controls::shuffle_btn, {
        { DN_BTNCLICK, &player::on_shuffle_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_shuffle_btn_style_applied },
    }},
    { controls::repeat_btn, {
        { DN_BTNCLICK, &player::on_repeat_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_repeat_btn_style_applied },
    }},
};

player::player(api_weak_ptr_t api, hotkeys_handler *hotkeys):
    api_proxy(api),
    hotkeys(hotkeys),
    volume(0, 100, 1),
    track_progress(0, 0, 5),
    shuffle_state({ true, false }),
    repeat_state({ playback_state_t::repeat_off, playback_state_t::repeat_track, playback_state_t::repeat_context })
    {}

player::~player()
{
    hide();

    api_proxy.reset();
}

bool player::handle_dlg_proc_event(intptr_t msg_id, intptr_t control_id, void *param)
{
    if (are_dlg_events_suppressed) return false;

    // first, trying to find a handler among the given control id event handers;
    // in negative scenario, trying to search for a handler among global handlers @NO_CONTROL id;
    // otherwise @false return control to the @dlg_proc function
    for (auto ctrl: { controls(control_id), controls::no_control })
    {
        auto it = dlg_event_handlers.find(ctrl);
        if (it != dlg_event_handlers.end())
        {
            for (auto &[mid, handler]: it->second)
                if (mid == msg_id)
                    return (this->*handler)(param);
        }
    }
    return false;
}

intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
{
    static player *dialog = nullptr;
    if (msg == DN_INITDIALOG)
    {
        dialog = reinterpret_cast<player*>(param2);
        return FALSE;
    }
    else if (msg == DN_CLOSE)
    {
        dialog->cleanup();
        dialog = nullptr;

        return TRUE;
    }

    if (dialog && dialog->handle_dlg_proc_event(msg, param1, param2))
        return TRUE;

    return config::ps_info.DefDlgProc(hdlg, msg, param1, param2);
}

bool player::show()
{
    if (!visible)
    {
        hdlg = config::ps_info.DialogInit(&MainGuid, &PlayerDialogGuid, -1, -1, width, height, L"Player",
            &dlg_items_layout[0], std::size(dlg_items_layout), 0, FDLG_SMALLDIALOG | FDLG_NONMODAL, &dlg_proc, this);
        are_dlg_events_suppressed = false;
        
        utils::events::start_listening<playback_observer>(this);
        utils::events::start_listening<devices_observer>(this);
        utils::events::start_listening<librespot_observer>(this);

        if (hdlg != NULL)
        {
            static wstring title(std::format(L" {} ", far3::get_text(MPluginUserName)));
            set_control_text(controls::title, title);

            if (auto api = api_proxy.lock())
            {
                const auto *devices_cache = api->get_devices_cache();
                const auto &state = api->get_playback_state();

                // TODO: invalidate devices and playback caches? instead of making blocking request each time, just
                // mark them invalidated, so the resync happens way faster

                on_track_changed(state.item, track_t{});
                on_track_progress_changed(state.item.duration, state.progress);
                on_volume_changed(state.device.volume_percent);
                on_state_changed(state.is_playing);
                on_context_changed(state.context);
                on_shuffle_state_changed(state.shuffle_state);
                on_repeat_state_changed(state.repeat_state);
                on_permissions_changed(state.actions);
        
                on_devices_changed(devices_cache->get_all());

                visible = true;
                
                return true;
            }
        }
    }
    return false;
}

bool player::hide()
{
    if (visible)
    {
        if (hdlg != NULL)
            far3::dialogs::close(hdlg);

        return true;
    }
    return false;
}

void player::cleanup()
{
    utils::events::stop_listening<playback_observer>(this);
    utils::events::stop_listening<devices_observer>(this);
    utils::events::stop_listening<librespot_observer>(this);
    
    hdlg = NULL;
    visible = false;
    are_dlg_events_suppressed = true;
}

void player::tick()
{
    if (!visible) return;
        
    // here we process delayed controls like seeking position or volume, as they require
    // timer for perform smoothly. But the final operation is executed through the far main
    // thread to avoid threads clashes

    track_progress.check([this](int p) {
        synchro_tasks::push([api = api_proxy.lock(), p] {
            const auto &state = api->get_playback_state();
            api->seek_to_position(p * 1000, state.device.id);
        }, "seek-to-position task");
    });

    volume.check([this](int v) {
        synchro_tasks::push(
            [api = api_proxy.lock(), v] { api->set_playback_volume(v); },
            "set-playback-volume task"
        );
    });

    shuffle_state.check([this](bool v) {
        synchro_tasks::push(
            [api = api_proxy.lock(), v] { api->toggle_shuffle(v); },
            "toggle-shuffle task"
        );
    });

    repeat_state.check([this](const string &s) {
        synchro_tasks::push(
            [api = api_proxy.lock(), s] { api->set_repeat_state(s); },
            "set-repeat-state task"
        );
    });
}

void player::on_seek_forward_btn_clicked()
{
    if (auto api = api_proxy.lock())
        if (const auto &pstate = api->get_playback_state())
            update_track_bar(track_progress.get_higher_boundary(), track_progress.next());
}

void player::on_seek_backward_btn_clicked()
{
    if (auto api = api_proxy.lock())
        if (const auto &pstate = api->get_playback_state())
            update_track_bar(track_progress.get_higher_boundary(), track_progress.prev());
}

void player::on_volume_up_btn_clicked()
{
    if (auto api = api_proxy.lock())
        if (const auto &pstate = api->get_playback_state())
            update_volume_bar(volume.next());
}

void player::on_volume_down_btn_clicked()
{
    if (auto api = api_proxy.lock())
        if (const auto &pstate = api->get_playback_state())
            update_volume_bar(volume.prev());
}

bool player::on_devices_item_selected(void *dialog_item)
{
    const auto *item = reinterpret_cast<FarDialogItem*>(dialog_item);

    auto device_id = far3::dialogs::get_list_current_item_data<string>(
        hdlg, controls::devices_combo);

    if (device_id.empty())
        return false;

    if (auto api = api_proxy.lock())
    {
        if (auto devices = api->get_devices_cache())
        {
            const auto &state = api->get_playback_state();
            devices->transfer_playback(device_id, state.is_playing);
        }
    }
    return true;
}

bool player::on_input_received(void *input_record)
{
    if (api_proxy.expired()) return false;

    auto api = api_proxy.lock();
    const auto &state = api->get_playback_state();
    const auto *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
    switch (ir->EventType)
    {
        case KEY_EVENT:
            if (ir->Event.KeyEvent.bKeyDown)
            {
                int key = keys::make_combined(ir->Event.KeyEvent);
                switch (key)
                {
                    case VK_SPACE:
                        on_play_btn_click();
                        return true;

                    case VK_RIGHT + keys::mods::alt:
                        on_skip_to_next_btn_click();
                        return true;

                    case VK_LEFT + keys::mods::alt:
                        on_skip_to_previous_btn_click();
                        return true;

                    case VK_LEFT:
                        on_seek_backward_btn_clicked();
                        return true;
                    
                    case VK_RIGHT:
                        on_seek_forward_btn_clicked();
                        return true;
                    
                    case VK_UP:
                        on_volume_up_btn_clicked();
                        return true;

                    case VK_DOWN:
                        on_volume_down_btn_clicked();
                        return true;

                    case VK_F8:
                        on_like_btn_input_received(NULL);
                        return true;
                    
                    case keys::r:
                        on_repeat_btn_click();
                        return true;
                    
                    case keys::s:
                        update_shuffle_btn(shuffle_state.next());
                        return true;
                    
                    case keys::d + keys::mods::alt:
                        if (is_control_enabled(controls::devices_combo))
                            far3::dialogs::open_list(hdlg, controls::devices_combo, true);
                        return true;

                    case keys::tilde + keys::mods::alt:
                        hide();
                        return true;
                }
            }
            break;
    }
    return false;
}

static bool get_playback_button_style(void *dialog_item_colors, bool is_enabled)
{
    auto *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    dic->Colors->BackgroundColor = colors::dgray;
    dic->Colors->ForegroundColor = is_enabled ? colors::black : colors::gray;
    return true;
}

bool player::on_play_btn_style_applied(void *dialog_item_colors)
{
    return get_playback_button_style(dialog_item_colors,
        is_control_enabled(controls::play_btn));
}

bool player::on_next_btn_style_applied(void *dialog_item_colors)
{
    return get_playback_button_style(dialog_item_colors,
        is_control_enabled(controls::next_btn));
}

bool player::on_prev_btn_style_applied(void *dialog_item_colors)
{
    return get_playback_button_style(dialog_item_colors,
        is_control_enabled(controls::prev_btn));
}

bool player::on_track_bar_style_applied(void *dialog_item_colors)
{
    auto *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    dic->Colors->ForegroundColor = colors::black;
    return true;
}

bool player::on_artist_label_input_received(void *input_record)
{
    const auto *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
    if (ir->EventType == KEY_EVENT)
        return false;

    // searching for a specific artist in the list of them, which
    // user has clicked on
    const SMALL_RECT &dlg_rect = utils::far3::dialogs::get_dialog_rect(hdlg);
    
    const auto &label_layout = dlg_items_layout[controls::artist_name];
    auto label_length = label_layout.X2 - label_layout.X1;
    auto click_pos = ir->Event.MouseEvent.dwMousePosition.X - (dlg_rect.Left + label_layout.X1);
    
    if (api_proxy.expired()) return false;

    auto api = api_proxy.lock();

    // we are iterating through all the names separated by comma in the full string,
    // and check whether the clicking position happened within range of symbols
    // of this particular name
    const auto &playback = api->get_playback_state();
    wstring ws = playback.item.album.get_artists_full_name();
    static std::wregex pattern(L"[^,]+");

    auto begin = std::wsregex_iterator{ ws.begin(), ws.end(), pattern };
    auto end = std::wsregex_iterator();

    wstring result;
    for (auto i = begin; i != end; ++i)
    {
        // if the position of the next match is farther than the clicking one,
        // our previous word is what we need
        if (i->position() > click_pos)
            break;
        
        result = i->str();
    }

    // if we won't find our artist by name, we pick the main one
    auto simplified_artist = playback.item.get_artist();
    for (const auto &a: playback.item.artists)
        if (a.name == utils::trim(result))
            simplified_artist = a;

    if (const auto &artist = api->get_artist(simplified_artist.id))
    {
        hide();

        ui::events::show_artist(api, artist);
        ui::events::select_item(playback.item.album.id);
    }

    return true;
}

bool player::on_track_label_input_received(void *input_record)
{
    const auto *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
    if (ir->EventType == KEY_EVENT)
        return false;

    if (api_proxy.expired()) return false;

    auto api = api_proxy.lock();
    const auto &playback = api->get_playback_state();

    if (const auto &a = playback.item.get_artist(); const auto &artist = api->get_artist(a.id))
    {
        hide();

        ui::events::show_album_tracks(api, playback.item.album);
        ui::events::select_item(playback.item.id);
    }

    return true;
}

bool player::on_source_label_input_received(void *input_record)
{
    const auto *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
    if (ir->EventType == KEY_EVENT)
        return false;
    
    if (auto api = api_proxy.lock())
    {
        const auto &state = api->get_playback_state();
        const auto &ctx = state.context;

        if (ctx.is_collection())
        {
            // showing a collection view
            ui::events::show_collection(api_proxy, multiview_builder_t::tracks_idx);
        }
        else if (ctx.is_artist())
        {
            // showing an artist's view with its albums
            if (const auto &artist = api->get_artist(ctx.get_item_id()))
                ui::events::show_artist(api, artist);
        }
        else if (ctx.is_album())
        {
            ui::events::show_album_tracks(api, state.item.album, std::bind(ui::events::show_root, api_proxy));
        }
        else if (ctx.is_playlist())
        {
            if (const auto &playlist = api->get_playlist(ctx.get_item_id()))
                ui::events::show_playlist(api, playlist);
        }
        
        ui::events::select_item(state.item.id);

        hide();

        return true;
    }

    return false;
}

bool player::on_track_bar_input_received(void *input_record)
{
    if (api_proxy.expired()) return false;

    auto api = api_proxy.lock();
    const auto &playback = api->get_playback_state();
    if (playback.is_empty())
        return false;

    const auto *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
    if (ir->EventType == KEY_EVENT)
        return false;

    const auto &dlg_rect = utils::far3::dialogs::get_dialog_rect(hdlg);
    const auto &track_bar_layout = dlg_items_layout[controls::track_bar];
    const auto track_bar_length = track_bar_layout.X2 - track_bar_layout.X1;
    const auto click_pos = ir->Event.MouseEvent.dwMousePosition.X - (dlg_rect.Left + track_bar_layout.X1);
    const auto progress_percent = (float)click_pos / track_bar_length;

    api->seek_to_position((int)std::round(playback.item.duration_ms * progress_percent));
    return true;
}

bool player::on_inactive_control_style_applied(void *dialog_item_colors)
{
    auto *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    dic->Colors->ForegroundColor = colors::dgray;
    return true;
}

bool player::on_like_btn_input_received(void *input_record)
{
    if (input_record)
    {
        const auto *ir = reinterpret_cast<INPUT_RECORD*>(input_record);
        if (ir->EventType == KEY_EVENT)
            return false;
    }

    if (api_proxy.expired()) return false;

    auto api = api_proxy.lock();
    const auto &playback = api->get_playback_state();
    if (playback.is_empty())
        return true;
    
    auto *library = api->get_library();
    bool is_saved = library->is_track_saved(playback.item.id, true);
    if (is_saved)
        library->remove_saved_tracks({ playback.item.id });
    else
        library->save_tracks({ playback.item.id });
    
    update_like_btn(is_saved);

    return true;
}

bool player::on_like_btn_style_applied(void *dialog_item_colors)
{
    if (api_proxy.expired()) return false;

    auto *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    auto api = api_proxy.lock();
    const auto &playback = api->get_playback_state();
    auto *library = api->get_library();
    if (!playback.is_empty() && library->is_track_saved(playback.item.id, true))
    {
        dic->Colors->ForegroundColor = colors::black;
    }
    else
    {
        dic->Colors->ForegroundColor = colors::dgray;
    }
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    return true;
}

bool player::on_shuffle_btn_style_applied(void *dialog_item_colors)
{
    auto *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    if (shuffle_state.get_offset_value())
    {
        dic->Colors->ForegroundColor = colors::black;
    }
    else
    {
        dic->Colors->ForegroundColor = colors::dgray;
    }
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    return true;
}

bool player::on_repeat_btn_style_applied(void *dialog_item_colors)
{
    auto *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    if (repeat_state.get_offset_value() != playback_state_t::repeat_off)
    {
        dic->Colors->ForegroundColor = colors::black;
    }
    else
    {
        dic->Colors->ForegroundColor = colors::dgray;
    }
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    return true;
}

bool player::on_skip_to_next_btn_click(void*)
{
    if (hotkeys && is_control_enabled(next_btn))
    {
        hotkeys->skip_to_next();
        return true;
    }
    return false;
}

bool player::on_skip_to_previous_btn_click(void*)
{
    if (hotkeys && is_control_enabled(prev_btn))
    {
        hotkeys->skip_to_prev();
        return true;
    }
    return false;
}

bool player::on_shuffle_btn_click(void *empty)
{
    if (!is_control_enabled(shuffle_btn)) return false;

    update_shuffle_btn(shuffle_state.next());
    return true;
}

bool player::on_repeat_btn_click(void *empty)
{
    if (auto api = api_proxy.lock())
    {
        const auto &pstate = api->get_playback_state();

        auto s = repeat_state.next();
    
        // if `repeat_track` selected and it is not permitted now, we skip it to next
        if (s == playback_state_t::repeat_track && !pstate.actions.toggling_repeat_track)
            s = repeat_state.next();
        
        // ...the same logic, skipping to next
        if (s == playback_state_t::repeat_context& !pstate.actions.toggling_repeat_context)
            s = repeat_state.next();
    
        if (s != pstate.repeat_state)
        {
            update_repeat_btn(s);
            return true;
        }
    }
    return false;
}

bool player::on_play_btn_click(void*)
{
    if (hotkeys && is_control_enabled(play_btn))
    {
        hotkeys->toggle_playback();
        return true;
    }
    return false;
}

void player::on_devices_changed(const devices_t &devices)
{
    no_redraw_player nr(hdlg);
    dlg_events_supressor s(this);

    far3::dialogs::clear_list(hdlg, controls::devices_combo);

    far3::dialogs::add_list_item(hdlg, controls::devices_combo, L"<no device>", 0, NULL, 0, false);

    for (size_t i = 0; i < devices.size(); i++)
    {
        const auto &dev = devices[i];
        far3::dialogs::add_list_item(hdlg, controls::devices_combo, dev.name, (int)i + 1,
            (void*)dev.id.c_str(), dev.id.size(), dev.is_active);
    }
}

void player::on_librespot_stopped(bool emergency)
{
    hide();
}

void player::on_track_changed(const track_t &track, const track_t &prev_track)
{
    no_redraw_player nr(hdlg);

    static wstring track_total_time_str;
    track_total_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(track.duration));

    track_progress.set_higher_boundary(track.duration);

    set_control_text(controls::track_name, track.name);
    set_control_text(controls::artist_name, track.album.get_artists_full_name());
    set_control_text(controls::track_total_time, track_total_time_str);

    if (auto api = api_proxy.lock())
    {
        const auto &state = api->get_playback_state();
        if (auto *library = api->get_library())
            update_like_btn(!state.is_empty() && library->is_track_saved(state.item.id, true));
    }
}

void player::update_track_bar(int duration, int progress)
{
    no_redraw_player nr(hdlg);

    static wstring track_bar, track_time_str;

    const auto &track_bar_layout = dlg_items_layout[controls::track_bar];
    const auto &track_bar_length = track_bar_layout.X2 - track_bar_layout.X1;
    track_bar = wstring(track_bar_length, track_bar_char_unfilled); // filling the bar with spare symbols first
    track_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(progress));

    if (duration)
    {
        float progress_percent = (float)progress / duration;
        int progress_chars_length = (int)std::round(track_bar_length * progress_percent);
        fill(track_bar.begin(), track_bar.begin() + progress_chars_length, track_bar_char_filled);
    }
    
    set_control_text(controls::track_bar, track_bar);
    set_control_text(controls::track_time, track_time_str);
}

void player::on_track_progress_changed(int duration, int progress)
{
    track_progress.set_value(progress);

    // prevents from updating, in case we are seeking a new track position,
    // which requires showing a virtual target track bar position
    if (!track_progress.is_waiting())
        update_track_bar((int)duration, (int)progress);
}

void player::update_volume_bar(int volume)
{
    no_redraw_player nr(hdlg);

    static wstring volume_label;
    
    volume_label = std::format(L"[{:<3}%]", volume);
    set_control_text(controls::volume_label, volume_label);
}

void player::on_volume_changed(int vol)
{
    volume.set_value(vol);

    // prevents from updating, in case we are seeking a new volume value,
    // which requires showing a virtual target vlume bar value
    if (volume.is_waiting())
        return;

    return update_volume_bar(vol);
}

void player::on_shuffle_state_changed(bool state)
{
    shuffle_state.set_value(state);

    if (shuffle_state.is_waiting())
        return;

    return update_shuffle_btn(state);
}

void player::update_shuffle_btn(bool is_shuffling)
{
    no_redraw_player nr(hdlg);
    set_control_text(controls::shuffle_btn, far3::get_text(MPlayerShuffleBtn));
}

void player::on_repeat_state_changed(const string &state)
{
    repeat_state.set_value(state);

    if (repeat_state.is_waiting())
        return;

    return update_repeat_btn(state);
}

void player::update_repeat_btn(const string &repeate_state)
{
    static const std::map<const string, const wchar_t*> labels{
        { playback_state_t::repeat_off, far3::get_text(MPlayerRepeatNoneBtn) },
        { playback_state_t::repeat_track, far3::get_text(MPlayerRepeatOneBtn) },
        { playback_state_t::repeat_context, far3::get_text(MPlayerRepeatAllBtn) },
    };

    no_redraw_player nr(hdlg);

    static wstring repeat_label;
    
    if (labels.contains(repeate_state))
        set_control_text(controls::repeat_btn, labels.at(repeate_state));
    else // unexpected situation, just showing what we've received
        set_control_text(controls::repeat_btn, utils::to_wstring(repeate_state));
}

void player::update_like_btn(bool is_saved)
{
    no_redraw_player nr(hdlg);
    set_control_text(controls::like_btn, like_btn_label);
}

void player::on_state_changed(bool is_playing)
{
    set_control_text(controls::play_btn, is_playing ? pause_btn_label : play_btn_label);
}

void player::on_context_changed(const context_t &ctx)
{
    if (api_proxy.expired()) return;

    auto api = api_proxy.lock();

    wstring source_label = L"";
    if (ctx.is_collection())
    {
        //source_name = far3::get_text(MPlayerSourceCollection);
    }
    else if (ctx.is_artist())
    {
        // source_name = L"artist";
    }
    else if (ctx.is_album())
    {
        if (auto album = api->get_album(ctx.get_item_id()))
        {
            wstring full_name = std::format(L"[{}] {}", utils::to_wstring(album.get_release_year()), album.name);
            if (album.is_single() || album.is_compilation())
                full_name += L" " + album.get_type_abbrev();
            
            source_label = std::format(L"Album: {}", full_name);
        }
    }
    else if (ctx.is_playlist())
    {
        if (auto playlist = api->get_playlist(ctx.get_item_id()))
            source_label = std::format(L"Playlist: {}", playlist.name);
    }
    
    if (source_label.empty())
        source_label = std::format(L"{}: {}", far3::get_text(MPlayerSourceLabel),
                                   utils::to_wstring(ctx.type));

    set_control_text(controls::source_name, source_label);
}

void player::on_permissions_changed(const actions_t &actions)
{
    if (auto api = api_proxy.lock())
    {
        const auto &state = api->get_playback_state();

        set_control_enabled(play_btn, state.is_playing ? actions.pausing : actions.resuming);
        set_control_enabled(track_bar, actions.seeking);
        set_control_enabled(prev_btn, actions.skipping_prev);
        set_control_enabled(next_btn, actions.skipping_next);
        set_control_enabled(shuffle_btn, actions.toggling_shuffle);
        set_control_enabled(devices_combo, actions.trasferring_playback);
    }
}

intptr_t player::set_control_text(int control_id, const wstring &text)
{
    return far3::dialogs::set_text(hdlg, control_id, text);
}

intptr_t player::set_control_enabled(int control_id, bool is_enabled)
{
    return far3::dialogs::enable(hdlg, control_id, is_enabled);
}

bool player::is_control_enabled(int control_id)
{
    return far3::dialogs::is_enabled(hdlg, control_id);
}

} // namespace ui
} // namespace spotifar