#ifndef LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#define LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "interfaces.hpp"
#include "cached_value.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;

        class LibraryCache: public ICachedData
        {
        public:
            LibraryCache(IApi *api);

            virtual void read(SettingsCtx &ctx);
            virtual void write(SettingsCtx &ctx);
            virtual void clear(SettingsCtx &ctx);
            virtual void resync(bool force = false);

        private:
            std::shared_ptr<spdlog::logger> logger;
            JsonStorageValue<ArtistsCollection> artists;
            IApi *api;
        };
    }
}

#endif // LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7