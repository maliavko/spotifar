#include "library.hpp"
#include "utils.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;

        LibraryCache::LibraryCache(IApi *api):
            logger(spdlog::get(utils::LOGGER_API)),
            api(api),
            followed_artists(L"FollowedArtists")
        {
            storages.assign({
                &followed_artists,
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
            if (!api->is_authenticated())
                return;

            if (!is_initialized)
            {
                is_initialized = true;

                logger->debug("Library initialization");

                ArtistsT result;
                json request_artists_url = httplib::append_query_params("/v1/me/following", {
                    { "type", "artist" },
                    { "limit", std::to_string(50) },
                });

                do
                {
                    if (auto r = api->get(request_artists_url))
                    {
                        json data = json::parse(r->body)["artists"];
                        request_artists_url = data["next"];

                        const auto &artists = data["items"].get<ArtistsT>();
                        result.insert(result.end(), artists.begin(), artists.end());
                    }
                }
                while (!request_artists_url.is_null());

                followed_artists.set(result);
            }
        }
    }
}