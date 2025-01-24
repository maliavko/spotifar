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
        
        std::generator<ArtistsT> LibraryCache::get_followed_artist(size_t limit)
        {
            json after = "";

            do
            {
                auto future = api->get_thread_pool().submit_task(
                    [&limit, &after, &client = api->get_client()] {
                        auto request_url = httplib::append_query_params("/v1/me/following", {
                            { "type", "artist" },
                            { "limit", std::to_string(limit) },
                            { "after", after.get<std::string>() },
                            { "sort", "alpha" },
                        });
                        
                        ArtistsT result;
                        if (auto r = client.Get(request_url))
                        {
                            json data = json::parse(r->body)["artists"];
                            for (auto [n,h]: r->headers)
                            {
                                if (n == "etag")
                                    spdlog::debug("etag {}", h);
                            }
                            after = data["cursors"]["after"];
                            data["items"].get_to(result);
                        }
                        return result;
                    });

                co_yield future.get();
            }
            while (!after.is_null());
        }
    }
}