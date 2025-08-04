#ifndef WAITING_HPP_ADC4A172_9906_457A_A486_B01AE5587085
#define WAITING_HPP_ADC4A172_9906_457A_A486_B01AE5587085
#pragma once

#include "stdafx.h"

namespace spotifar { namespace ui {

/// @brief Splash screen-dialog, shows long operations progress
class waiting
{
public:
    static void show(const wstring &msg);

    static void hide();

    static void tick(const utils::clock_t::duration &delta);
private:
    waiting() {}
    ~waiting() {}
};

} // namespace ui
} // namespace spotifar

#endif // WAITING_HPP_ADC4A172_9906_457A_A486_B01AE5587085