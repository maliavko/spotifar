#include "ui/player.hpp"

namespace spotifar { namespace ui {

namespace far3 = utils::far3;
namespace keys = far3::keys;
namespace colors = far3::colors;
namespace synchro_tasks = far3::synchro_tasks;

static const wchar_t
    track_bar_char_unfilled = 0x2591,
    track_bar_char_filled = 0x2588,
    *play_btn_label = L"[ \x25ba ]",
    *pause_btn_label = L"[ ‖ ]",
    *next_btn_label = L"[>>]",
    *prev_btn_label = L"[<<]",
    *like_btn_label = L"[+]";

static const int
    width = 60, height = 10,
    view_x = 2, view_y = 2, view_width = width - 2, view_height = height - 2,
    view_center_x = (view_width + view_x)/2, view_center_y = (view_height + view_y)/2;

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

/// @brief a helper class to suppress `dlg_proc` function from handling incoming events,
/// while the instance of the class exists in the particular scope
struct [[nodiscard]] dlg_events_supressor
{
    bool were_events_suppressed;
    player *dialog;

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
};

/// @brief A helper class to avoid redrawing in the function scope,
/// usually while the object exists
struct [[nodiscard]] no_redraw
{
    no_redraw(HANDLE h): hdlg(h)
    {
        assert(hdlg);
        std::lock_guard lock(mutex);
        far3::msg::send(hdlg, DM_ENABLEREDRAW, FALSE, 0);
    }

    ~no_redraw()
    {
        std::lock_guard lock(mutex);
        far3::msg::send(hdlg, DM_ENABLEREDRAW, TRUE, 0);
    }
    
    HANDLE hdlg;
    inline static std::mutex mutex{};
};

static FarDialogItem control(FARDIALOGITEMTYPES type, intptr_t x1, intptr_t y1, intptr_t x2, intptr_t y2,
                             FARDIALOGITEMFLAGS flags, const wchar_t *data = L"")
{
    return FarDialogItem(type, x1, y1, x2, y2, {}, nullptr, nullptr, flags, data);
}

static const auto
    btn_flags = DIF_NOBRACKETS | DIF_NOFOCUS | DIF_BTNNOCLOSE,
    combo_flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;

static const std::vector<FarDialogItem> dlg_items_layout{
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
    control(DI_BUTTON,      view_center_x - 2, view_height, 1, 1,           btn_flags, play_btn_label), // PLAY
    control(DI_BUTTON,      view_center_x - 7, view_height, 1, 1,           btn_flags, prev_btn_label), // PREV_BTN
    control(DI_BUTTON,      view_center_x + 4, view_height, 1, 1,           btn_flags, next_btn_label), // NEXT_BTN
    control(DI_BUTTON,      view_x, view_height, 1, 1,                      btn_flags, like_btn_label), // LIKE BTN
    control(DI_TEXT,        view_width - 6, view_height, 1, 1,              btn_flags | DIF_RIGHTTEXT, L"[---%]"), // VOLUME
    control(DI_BUTTON,      view_center_x + 9, view_height, 1, 1,           btn_flags),  // REPEAT
    control(DI_BUTTON,      view_center_x - 15, view_height, 1, 1,          btn_flags),  // SHUFFLE
    
    // devices box
    control(DI_COMBOBOX,    view_width - 13, 1, view_width - 1, 0,          combo_flags), // DEVICES
};

typedef bool (player::*control_handler_t)(void*);
static const std::map<controls, std::map<FARMESSAGE, control_handler_t>> dlg_event_handlers{
    { controls::no_control, {
        { DN_CONTROLINPUT, &player::on_input_received },
    }},
    { controls::devices_combo, {
        { DN_EDITCHANGE, &player::on_devices_item_selected },
    }},
    { controls::next_btn, {
        { DN_BTNCLICK, &player::on_skip_to_next_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_playback_control_style_applied },
    }},
    { controls::prev_btn, {
        { DN_BTNCLICK, &player::on_skip_to_previous_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_playback_control_style_applied },
    }},
    { controls::play_btn, {
        { DN_BTNCLICK, &player::on_play_btn_click },
        { DN_CTLCOLORDLGITEM, &player::on_playback_control_style_applied },
    }},
    { controls::track_bar, {
        { DN_CTLCOLORDLGITEM, &player::on_track_bar_style_applied },
        { DN_CONTROLINPUT, &player::on_track_bar_input_received },
    }},
    { controls::artist_name, {
        { DN_CTLCOLORDLGITEM, &player::on_inactive_control_style_applied },
    }},
    { controls::source_name, {
        { DN_CTLCOLORDLGITEM, &player::on_inactive_control_style_applied },
    }},
    { controls::like_btn, {
        { DN_CTLCOLORDLGITEM, &player::on_inactive_control_style_applied },
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

player::player(spotify::api &api):
    api(api),
    volume(0, 100, 1),
    track_progress(0, 0, 5),
    shuffle_state({ true, false }),
    repeat_state({
        playback_state::repeat_off,
        playback_state::repeat_track,
        playback_state::repeat_context })
{
}

player::~player()
{
    hide();
}

bool player::handle_dlg_proc_event(intptr_t msg_id, intptr_t control_id, void *param)
{
    if (are_dlg_events_suppressed)
        return false;

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
    static player* dialog = nullptr;
    if (msg == DN_INITDIALOG)
    {
        dialog = reinterpret_cast<player*>(param2);
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

    return config::ps_info.DefDlgProc(hdlg, msg, param1, param2);
}

bool player::show()
{
    if (!visible)
    {
        hdlg = config::ps_info.DialogInit(&MainGuid, &PlayerDialogGuid, -1, -1, width, height, 0,
            &dlg_items_layout[0], std::size(dlg_items_layout), 0, FDLG_SMALLDIALOG | FDLG_NONMODAL, &dlg_proc, this);
        are_dlg_events_suppressed = false;
        
        // api.start_listening(dynamic_cast<BasicApiObserver*>(this));
        api.start_listening(dynamic_cast<playback_observer*>(this));
        api.start_listening(dynamic_cast<devices_observer*>(this));

        if (hdlg != NULL)
        {
            visible = true;
            
            static wstring title(std::format(L" {} ", far3::get_text(MPluginUserName)));
            set_control_text(controls::title, title);

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

bool player::hide(bool close_ui)
{
    if (visible)
    {
        // api.stop_listening(dynamic_cast<BasicApiObserver*>(this));
        api.stop_listening(dynamic_cast<playback_observer*>(this));
        api.stop_listening(dynamic_cast<devices_observer*>(this));

        if (hdlg != NULL && close_ui)
            far3::msg::close(hdlg);
        
        hdlg = NULL;
        visible = false;
        are_dlg_events_suppressed = true;

        return true;
    }
    return false;
}

void player::tick()
{
    // here we process delayed controls like seeking position or volume, as they require
    // timer for perform smoothly. But the final operation is executed through the far main
    // thread to avoid threads clashes

    track_progress.check([this](int p) {
        synchro_tasks::push([&api = this->api, p] {
            auto &state = api.get_playback_state();
            api.seek_to_position(p * 1000, state.device.id);
        });
    });

    volume.check([this](int v) {
        synchro_tasks::push([&api = this->api, v] { api.set_playback_volume(v); });
    });

    shuffle_state.check([this](bool v) {
        synchro_tasks::push([&api = this->api, v] { api.toggle_shuffle(v); });
    });

    repeat_state.check([this](const string &s) {
        synchro_tasks::push([&api = this->api, s] { api.set_repeat_state(s); });
    });
}

bool player::on_devices_item_selected(void *dialog_item)
{
    FarDialogItem *item = reinterpret_cast<FarDialogItem*>(dialog_item);

    size_t pos = far3::msg::get_list_current_pos(hdlg, controls::devices_combo);
    auto device_id = far3::msg::get_list_item_data<string>(hdlg, controls::devices_combo, pos);

    if (!device_id.empty())
    {
        auto &state = api.get_playback_state();
        api.transfer_playback(device_id, state.is_playing);
    }
    return true;
}

bool player::on_input_received(void *input_record)
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
                        on_play_btn_click();
                        return true;

                    case VK_RIGHT + keys::mods::alt:
                        on_skip_to_next_btn_click();
                        return true;

                    case VK_LEFT + keys::mods::alt:
                        on_skip_to_previous_btn_click();
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
                    case keys::r:
                    {
                        update_repeat_btn(repeat_state.next());
                        return true;
                    }
                    case keys::s:
                    {
                        update_shuffle_btn(shuffle_state.next());
                        return true;
                    }
                    case keys::s + keys::mods::shift:
                    {
                        api.toggle_shuffle_plus(true);
                        return true;
                    }
                    case keys::d + keys::mods::alt:
                    {
                        far3::msg::open_list(hdlg, controls::devices_combo, true);
                        return true;
                    }
                }
            }
            break;
    }
    return false;
}

bool player::on_playback_control_style_applied(void *dialog_item_colors)
{
    FarDialogItemColors* dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    dic->Colors->BackgroundColor = colors::dgray;
    dic->Colors->ForegroundColor = colors::black;
    return true;
}

bool player::on_track_bar_style_applied(void *dialog_item_colors)
{
    FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    dic->Colors->ForegroundColor = colors::black;
    return true;
}

bool player::on_track_bar_input_received(void *input_record)
{
    auto &playback = api.get_playback_state();
    if (playback.is_empty())
        return false;

    INPUT_RECORD *ir = reinterpret_cast<INPUT_RECORD*>(input_record);

    SMALL_RECT dlg_rect;
    far3::msg::send(hdlg, DM_GETDLGRECT, 0, &dlg_rect);
    
    auto track_bar_layout = dlg_items_layout[controls::track_bar];
    auto track_bar_length = track_bar_layout.X2 - track_bar_layout.X1;
    auto click_pos = ir->Event.MouseEvent.dwMousePosition.X - (dlg_rect.Left + track_bar_layout.X1);
    auto progress_percent = (float)click_pos / track_bar_length;

    api.seek_to_position((int)(playback.item.duration_ms * progress_percent));
    return true;
}

bool player::on_inactive_control_style_applied(void *dialog_item_colors)
{
    FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    dic->Flags = FCF_BG_INDEX | FCF_FG_INDEX;
    dic->Colors->ForegroundColor = colors::dgray;
    return true;
}

bool player::on_shuffle_btn_style_applied(void *dialog_item_colors)
{
    FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
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
    FarDialogItemColors *dic = reinterpret_cast<FarDialogItemColors*>(dialog_item_colors);
    if (repeat_state.get_offset_value() != playback_state::repeat_off)
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

bool player::on_skip_to_next_btn_click(void *empty)
{
    api.skip_to_next();
    return true;
}

bool player::on_skip_to_previous_btn_click(void *empty)
{
    api.skip_to_previous();
    return true;
}

bool player::on_shuffle_btn_click(void *empty)
{
    update_shuffle_btn(shuffle_state.next());
    return true;
}

bool player::on_repeat_btn_click(void *empty)
{
    update_repeat_btn(repeat_state.next());
    return true;
}

bool player::on_play_btn_click(void *empty)
{
    // // TODO: handle errors
    // auto &playback = api.get_playback_state();
    // if (playback.is_playing)
    // {
    //     api.pause_playback();
    //     return true;
    // }
    // else if (!playback.is_empty())
    // {
    //     if (!playback.context.uri.empty())
    //         api.start_playback(playback.context.uri, playback.item.get_uri(),
    //             playback.progress_ms, playback.device.id);
    //     else
    //         api.resume_playback(playback.device.id);
    //     return true;
    // }
    api.toggle_playback();
    return true;
}

void player::on_playback_sync_finished(const string &exit_msg)
{
    if (!exit_msg.empty())
        far3::show_far_error_dlg(MFarMessageErrorPlaybackSync, exit_msg);
    
    hide();
}

void player::on_devices_changed(const devices_list_t &devices)
{
    no_redraw nr(hdlg);
    dlg_events_supressor s(this);

    far3::msg::clear_list(hdlg, controls::devices_combo);

    for (int i = 0; i < devices.size(); i++)
    {
        auto &dev = devices[i];
        far3::msg::add_list_item(hdlg, controls::devices_combo, dev.name, i,
                            (void*)dev.id.c_str(), dev.id.size(), dev.is_active);
    }
}

void player::on_track_changed(const Track &track)
{
    no_redraw nr(hdlg);

    static wstring track_total_time_str;
    track_total_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(track.duration));

    auto &state = api.get_playback_state();
    track_progress.set_higher_boundary(track.duration);

    set_control_text(controls::track_name, track.name);
    set_control_text(controls::artist_name, track.artists.size() ? track.artists[0].name : L"");
    set_control_text(controls::track_total_time, track_total_time_str);
}

void player::update_track_bar(int duration, int progress)
{
    no_redraw nr(hdlg);

    static wstring track_bar, track_time_str;

    auto track_bar_layout = dlg_items_layout[controls::track_bar];
    auto track_bar_size = track_bar_layout.X2 - track_bar_layout.X1;
    track_bar = wstring(track_bar_size, track_bar_char_unfilled);
    track_time_str = std::format(L"{:%M:%S}", std::chrono::seconds(progress));

    if (duration)
    {
        float progress_percent = (float)progress / duration;
        int progress_chars_length = (int)(track_bar_size * progress_percent);
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
    if (track_progress.is_waiting())
        return;

    return update_track_bar((int)duration, (int)progress);
}

void player::update_volume_bar(int volume)
{
    no_redraw nr(hdlg);

    static wstring volume_label;
    
    volume_label = std::format(L"[{}%]", volume);
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
    no_redraw nr(hdlg);

    static wstring shuffle_label;

    shuffle_label = utils::utf8_decode("Shuffle");
    set_control_text(controls::shuffle_btn, shuffle_label);
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
    no_redraw nr(hdlg);

    static wstring repeat_label;
    
    // TODO: localize?
    repeat_label = utils::utf8_decode(repeate_state);
    set_control_text(controls::repeat_btn, repeat_label);
}

void player::on_state_changed(bool is_playing)
{
    set_control_text(controls::play_btn, is_playing ? pause_btn_label : play_btn_label);
}

void player::on_context_changed(const context &ctx)
{
    wstring source_name = L"";
    if (ctx.is_collection())
    {
        source_name = far3::get_text(MPlayerSourceCollection);
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
    
    static wstring source_label;
    if (!source_name.empty())
        source_label = std::format(L"{}: {}", far3::get_text(MPlayerSourceLabel), source_name);
    else
        source_label = L"";

    set_control_text(controls::source_name, source_label);
}

void player::on_permissions_changed(const spotify::actions &actions)
{
    // TODO: finish the content
}

intptr_t player::set_control_text(int control_id, const wstring &text)
{
    return far3::msg::set_text(hdlg, control_id, text);
}

intptr_t player::set_control_enabled(int control_id, bool is_enabled)
{
    return far3::msg::enable(hdlg, control_id, is_enabled);
}

} // namespace ui
} // namespace spotifar