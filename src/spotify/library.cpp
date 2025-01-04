#include "library.hpp"
#include "utils.hpp"

namespace spotifar
{
    namespace spotify
    {
        LibraryCache::LibraryCache(IApi *api):
            logger(spdlog::get(utils::LOGGER_API)),
            artists(L"Artists"),
            api(api)
        {

        }

        void LibraryCache::read(SettingsCtx &ctx)
        {

        }

        void LibraryCache::write(SettingsCtx &ctx)
        {

        }

        void LibraryCache::clear(SettingsCtx &ctx)
        {

        }

        void LibraryCache::resync(bool force)
        {

        }
    }
}