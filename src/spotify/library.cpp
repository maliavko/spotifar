#include "library.hpp"
#include "utils.hpp"

namespace spotifar
{
    namespace spotify
    {
        LibraryCache::LibraryCache(IApi *api):
            logger(spdlog::get(utils::LOGGER_API)),
            api(api),
            artists(L"Artists")
        {
            storages.assign({
                &artists
            });
        }

        LibraryCache::~LibraryCache()
        {
            storages.clear();
        }

        void LibraryCache::read(SettingsCtx &ctx)
        {
            for (auto &s: storages)
                s->read(ctx);
        }

        void LibraryCache::write(SettingsCtx &ctx)
        {
            for (auto &s: storages)
                s->write(ctx);
        }

        void LibraryCache::clear(SettingsCtx &ctx)
        {
            for (auto &s: storages)
                s->clear(ctx);
        }

        void LibraryCache::resync(bool force)
        {

        }
    }
}