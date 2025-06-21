#ifndef SEARCH_RESULTS_HPP_6686E1FF_3AF1_450D_906B_D26DE3B9291D
#define SEARCH_RESULTS_HPP_6686E1FF_3AF1_450D_906B_D26DE3B9291D
#pragma once

#include "dialog.hpp"
#include "spotify/requesters.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar { namespace ui {

class search_results_dialog:
    public modal_dialog,
    public spotify::collection_observer
{
    struct item_entry
    {
        using handler_t = std::function<void(spotify::api_weak_ptr_t, const spotify::data_item_t &)>;

        wstring label;
        LISTITEMFLAGS flags;
        const spotify::data_item_t *data;
        handler_t show_handler{};
        handler_t play_handler{};
    };
public:
    search_results_dialog(const spotify::search_requester &r);
    ~search_results_dialog();
protected:
    void rebuild_items();
    void refresh_list(const std::unordered_set<spotify::item_id_t> &ids);

    // modal_dialog
    void init() override;
    auto handle_result(intptr_t dialog_run_result) -> intptr_t override;
    bool handle_key_pressed(int ctrl_id, int combined_key) override;

    // collection_observer
    void on_tracks_statuses_received(const spotify::item_ids_t &ids) override;
    void on_artists_statuses_received(const spotify::item_ids_t &ids) override;
    void on_albums_statuses_received(const spotify::item_ids_t &ids) override;
private:
    const spotify::search_requester &requester;
    std::vector<item_entry> items;
};

} // namespace ui
} // namespace spotifar

#endif // SEARCH_RESULTS_HPP_6686E1FF_3AF1_450D_906B_D26DE3B9291D