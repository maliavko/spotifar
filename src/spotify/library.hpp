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
            virtual ~LibraryCache();

            // storable data interface
            virtual void read(SettingsCtx &ctx);
            virtual void write(SettingsCtx &ctx);
            virtual void clear(SettingsCtx &ctx);

            std::generator<ArtistsT> get_followed_artist(size_t limit);

            // cached data interface
            virtual void resync(bool force = false);

        private:
            std::shared_ptr<spdlog::logger> logger;
            IApi *api;
            
            std::vector<IStorableData*> storages;
            JsonStorageValue<ArtistsCollection> artists;
        };
    }
}

#endif // LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7