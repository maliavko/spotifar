#include "library.hpp"
#include "utils.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace utils;

        LibraryCache::LibraryCache(api_abstract *api):
            api(api)
        {
        }

        LibraryCache::~LibraryCache()
        {
            storages.clear();
        }

        void LibraryCache::read(settings_ctx &ctx)
        {
            for (auto &s: storages)
                s->read(ctx);
        }

        void LibraryCache::write(settings_ctx &ctx)
        {
            for (auto &s: storages)
                s->write(ctx);
        }

        void LibraryCache::clear(settings_ctx &ctx)
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

                log::api->debug("Library initialization");

                followed_artists.clear();
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
                        followed_artists.insert(followed_artists.end(), artists.begin(), artists.end());
                    }
                }
                while (!request_artists_url.is_null());
            }
        }
    }
}