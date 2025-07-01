#ifndef ALBUMS_HPP_B185DAAE_E5A1_40FC_9C20_EE7C45320453
#define ALBUMS_HPP_B185DAAE_E5A1_40FC_9C20_EE7C45320453
#pragma once

#include "stdafx.h"
#include "ui/dialogs/dialog.hpp"

namespace spotifar { namespace ui { namespace filters {

/// @brief A filtering dialog for albums views, offeres a possibility
/// to filter dialogs by albums' types: LPs, EPs, appears-on or compilations
class albums_filters_dialog: public modal_dialog
{
public:
    albums_filters_dialog();
protected:
    void init() override;
    auto handle_result(intptr_t dialog_run_result) -> intptr_t override;
};

} // namespace filter
} // namespace ui
} // namespace spotifar

#endif // ALBUMS_HPP_B185DAAE_E5A1_40FC_9C20_EE7C45320453