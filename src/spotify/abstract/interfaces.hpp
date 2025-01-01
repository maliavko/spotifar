#ifndef INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6

#include "stdafx.h"
#include "config.hpp"

namespace spotifar
{
    namespace spotify
    {
        using SettingsCtx = config::SettingsContext;

        struct IApi
        {
            virtual ~IApi() {}
        };

        struct ICachedValue
        {
            virtual ~ICachedValue() {};

            virtual void read(SettingsCtx &ctx) = 0;
            virtual void write(SettingsCtx &ctx) = 0;
            virtual void clear(SettingsCtx &ctx) = 0;
            virtual bool resync(bool force = false) = 0;
            virtual void set_enabled(bool is_enabled) = 0;
        };
    }
}
#endif // INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6