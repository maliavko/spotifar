#ifndef INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"

namespace spotifar
{
    namespace spotify
    {
        struct IApi
        {
            virtual ~IApi() {}
            virtual httplib::Client& get_client() = 0;
        };

        struct ICachedData: public utils::far3::IStorableData
        {
            virtual void resync(bool force = false) = 0;
        };
    }
}
#endif // INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6