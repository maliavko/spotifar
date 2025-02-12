#ifndef PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3
#define PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3

#include "stdafx.h"
#include "spotify/api.hpp"
#include "controls.hpp"

namespace spotifar { namespace ui {

using namespace spotify;

class player:
    public playback_observer,
    public devices_observer
{
public:
    player(api &api);
    virtual ~player();

    bool show();
    bool hide();
    void cleanup();
    void tick();

    inline bool is_visible() const { return visible; }

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

    // controls' styles
    bool on_playback_control_style_applied(void *dialog_item_colors);
    bool on_track_bar_style_applied(void *dialog_item_colors);
    bool on_inactive_control_style_applied(void *dialog_item_colors);
    bool on_shuffle_btn_style_applied(void *dialog_item_colors);
    bool on_repeat_btn_style_applied(void *dialog_item_colors);

protected:
    friend intptr_t WINAPI dlg_proc(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2);
    bool handle_dlg_proc_event(intptr_t msg_id, intptr_t control_id, void *param);
    void update_track_bar(int duration, int progress);
    void update_volume_bar(int volume);
    void update_shuffle_btn(bool is_shuffling);
    void update_repeat_btn(const string &repeate_state);

    // api even handlers
    virtual void on_playback_sync_finished(const string &err_msg);
    virtual void on_devices_changed(const devices_list_t &devices);
    virtual void on_track_changed(const track &track);
    virtual void on_track_progress_changed(int duration, int progress);
    virtual void on_volume_changed(int volume);
    virtual void on_shuffle_state_changed(bool state);
    virtual void on_repeat_state_changed(const string &state);
    virtual void on_state_changed(bool is_playing);
    virtual void on_context_changed(const context &ctx);
    virtual void on_permissions_changed(const spotify::actions &actions);

    // helpers
    intptr_t set_control_text(int control_id, const wstring &text);
    intptr_t set_control_enabled(int control_id, bool is_enabled);

private:
    api &api;
    HANDLE hdlg;
    bool visible = false;
    bool are_dlg_events_suppressed = true;

    slider_control volume, track_progress;
    cycled_bool_control shuffle_state;
    cycled_string_control repeat_state;
    
    friend struct dlg_events_supressor;
};
    
} // namespace ui
} // namespace spotifar

#endif //PLAYER_DIALOG_HPP_C5FAC22D_B92B_41D1_80F0_B5A6F708F0C3