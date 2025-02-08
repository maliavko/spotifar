#ifndef LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#define LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7
#pragma once

#include "stdafx.h"
#include "items.hpp"
#include "abstract.hpp"
#include "cache.hpp"

namespace spotifar
{
    namespace spotify
    {
        class LibraryCache: public cached_data_abstract
        {
        public:
            LibraryCache(api_abstract *api);
            virtual ~LibraryCache();

            // persistent data interface
            virtual void read(settings_ctx &ctx);
            virtual void write(settings_ctx &ctx);
            virtual void clear(settings_ctx &ctx);

            const ArtistsT& get_followed_artist() { return followed_artists; }

            // cached data interface
            virtual void resync(bool force = false);

        private:
            bool is_initialized = false;
            api_abstract *api;
            
            std::vector<config::persistent_data_abstract*> storages;

            ArtistsT followed_artists;
        };
    }
}

#endif // LIBRARY_HPP_3A547966_08A2_4D08_AE29_07076848E8F7