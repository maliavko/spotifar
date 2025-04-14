#ifndef PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3
#define PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3

#include "stdafx.h"
#include "spotify/abstract.hpp"
#include "spotify/playback.hpp"
#include "spotify/devices.hpp"
#include "controls.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class player:
    public playback_observer, // represent timely playback changes in UI
    public devices_observer // to keep up to date the list of available devices
{
    friend struct dlg_events_supressor; // a helper to supress processing of the events
                                        // by dialog for some cases
public:
    player(api_proxy_ptr api);
    ~player();

    bool show();
    bool hide();
    void cleanup();
    void tick();

    bool is_visible() const { return visible; }
    bool is_expanded() const;
    void expand(bool is_unfolded);

    // a set of public methods used by global 
    void on_seek_forward_btn_clicked();
    void on_seek_backward_btn_clicked();
    void on_volume_up_btn_clicked();
    void on_volume_down_btn_clicked();

    // controls' event handlers
    bool on_skip_to_next_btn_click(void* = nullptr);
    bool on_skip_to_previous_btn_click(void* = nullptr);
    bool on_play_btn_click(void* = nullptr);
    bool on_shuffle_btn_click(void* = nullptr);
    bool on_repeat_btn_click(void* = nullptr);
    bool on_devices_item_selected(void *dialog_item);
    bool on_input_received(void *input_record);
    bool on_artist_label_input_received(void *input_record);
    bool on_source_label_input_received(void *input_record);
    bool on_track_label_input_received(void *input_record);
    bool on_track_bar_input_received(void *input_record);
    bool on_like_btn_input_received(void *input_record);
    bool on_playing_queue_input_received(void *input_record);

    // controls' styles
    bool on_playback_control_style_applied(void *dialog_item_colors);
    bool on_track_bar_style_applied(void *dialog_item_colors);
    bool on_inactive_control_style_applied(void *dialog_item_colors);
    bool on_shuffle_btn_style_applied(void *dialog_item_colors);
    bool on_repeat_btn_style_applied(void *dialog_item_colors);
    bool on_like_btn_style_applied(void *dialog_item_colors);

protected:
    friend intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2);
    bool handle_dlg_proc_event(intptr_t msg_id, intptr_t control_id, void *param);
    void update_track_bar(int duration, int progress);
    void update_volume_bar(int volume);
    void update_shuffle_btn(bool is_shuffling);
    void update_repeat_btn(const string &repeate_state);
    void update_like_btn(bool is_saved);
    void update_playing_queue(bool is_visible);

    // api even handlers
    void on_playback_sync_finished(const string &err_msg);
    void on_devices_changed(const devices_t &devices);
    void on_track_changed(const track_t &track);
    void on_track_progress_changed(int duration, int progress);
    void on_volume_changed(int volume);
    void on_shuffle_state_changed(bool state);
    void on_repeat_state_changed(const string &state);
    void on_state_changed(bool is_playing);
    void on_context_changed(const context_t &ctx);
    void on_permissions_changed(const spotify::actions_t &actions);

    // helpers
    intptr_t set_control_text(int control_id, const wstring &text);
    intptr_t set_control_enabled(int control_id, bool is_enabled);

private:
    api_proxy_ptr api_proxy;
    HANDLE hdlg;
    std::atomic<bool> visible = false;
    bool are_dlg_events_suppressed = true;

    // some cutom logic, incapsulated into virtual ui controls
    slider_control volume, track_progress;
    cycled_bool_control shuffle_state;
    cycled_string_control repeat_state;
};
    
} // namespace ui
} // namespace spotifar

#endif //PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3