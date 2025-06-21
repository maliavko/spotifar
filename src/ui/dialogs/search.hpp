#ifndef SEARCH_HPP_47A92667_5CCC_4238_8580_E7A8D795E6D0
#define SEARCH_HPP_47A92667_5CCC_4238_8580_E7A8D795E6D0
#pragma once

#include "dialog.hpp"
#include "config.hpp"

namespace spotifar { namespace ui {

class search_dialog: public modal_dialog
{
public:
    search_dialog();
protected:
    // modal_dialog
    void init() override;
    auto handle_result(intptr_t dialog_run_result) -> intptr_t override;
    bool handle_btn_clicked(int ctrl_id, std::uintptr_t param) override;
private:
    config::settings::search_dialog_t *settings;
};

} // namespace ui
} // namespace spotifar

#endif // SEARCH_HPP_47A92667_5CCC_4238_8580_E7A8D795E6D0