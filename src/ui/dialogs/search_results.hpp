#ifndef SEARCH_RESULTS_HPP_6686E1FF_3AF1_450D_906B_D26DE3B9291D
#define SEARCH_RESULTS_HPP_6686E1FF_3AF1_450D_906B_D26DE3B9291D
#pragma once

#include "dialog.hpp"
#include "spotify/requesters.hpp"

namespace spotifar { namespace ui {

class search_results_dialog: public modal_dialog
{
public:
    search_results_dialog(const spotify::search_requester &r);
    ~search_results_dialog();
protected:
    // modal_dialog
    void init() override;
    auto handle_result(intptr_t dialog_run_result) -> intptr_t;
private:
    const spotify::search_requester &requester;
};

} // namespace ui
} // namespace spotifar

#endif // SEARCH_RESULTS_HPP_6686E1FF_3AF1_450D_906B_D26DE3B9291D