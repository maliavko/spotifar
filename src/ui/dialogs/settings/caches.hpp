#ifndef CACHES_HPP_2FE1C7CC_0C2E_4D37_AA79_49114D82E549
#define CACHES_HPP_2FE1C7CC_0C2E_4D37_AA79_49114D82E549
#pragma once

#include "stdafx.h"
#include "ui/dialogs/dialog.hpp"
#include "spotify/observer_protocols.hpp"

namespace spotifar { namespace ui { namespace settings {

class caches_dialog:
    public modal_dialog,
    public spotify::releases_observer
{
public:
    caches_dialog();
    ~caches_dialog();
protected:
    // modal_dialog
    void init() override;
    bool handle_btn_clicked(int ctrl_id) override;

    // releases_observer handlers
    void on_sync_progress_changed(size_t items_left) override;
};

} // namespace settings
} // namespace ui
} // namespace spotifar

#endif // CACHES_HPP_2FE1C7CC_0C2E_4D37_AA79_49114D82E549