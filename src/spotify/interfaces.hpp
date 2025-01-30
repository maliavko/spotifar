#ifndef INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6

#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"
#include "observers.hpp"

namespace spotifar
{
    namespace spotify
    {
        struct IApi
        {
            virtual ~IApi() {}
            virtual httplib::Client& get_client() = 0;
            virtual BS::thread_pool& get_thread_pool() = 0;
            virtual bool is_authenticated() const = 0;
            virtual size_t get_playback_observers_count() const = 0;
            virtual httplib::Result get(const string &request_url, utils::clock::duration cache_for = {}) = 0;
        };

        struct ICachedData: public utils::far3::IStorableData
        {
            virtual void resync(bool force = false) = 0;
            virtual bool is_enabled() const { return true; }
        };
    }
}
#endif // INTERFACES_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6