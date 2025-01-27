#include "library.hpp"
#include "utils.hpp"

namespace spotifar
{
    namespace spotify
    {
        LibraryCache::LibraryCache(IApi *api):
            logger(spdlog::get(utils::LOGGER_API)),
            api(api),
            followed_artists(L"FollowedArtists"),
            followed_artists_etags(L"FollowedArtistsETags")
        {
            storages.assign({
                &followed_artists,
                &followed_artists_etags,
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
            //followed_artists_etags.set({});
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

                json after = "";
                auto etags = followed_artists_etags.get();
                ArtistsT result;
                size_t offset = 0;

                do
                {
                    auto request_url = httplib::append_query_params("/v1/me/following", {
                        { "type", "artist" },
                        { "limit", std::to_string(50) },
                        { "after", after.get<string>() },
                    });

                    string etag = "";
                    if (etags.contains(after.get<string>()))
                        etag = etags.at(after.get<string>());
                    
                    //if (auto r = api->get_client().Get(request_url, {{ "If-None-Match", etag }}))
                    if (auto r = api->get(request_url))
                    {
                        if (r->status == httplib::NotModified_304)
                        {
                            const auto &cached = followed_artists.get();
                            size_t count = std::min(cached.size() - offset, 50ULL);
                            result.insert(result.end(), cached.begin() + offset, cached.begin() + offset + count);
                            offset += count;

                            if (offset == cached.size())
                                after = nullptr;
                            else
                                after = result.back().id;
                        }
                        else if (r->status == httplib::OK_200)
                        {
                            json data = json::parse(r->body)["artists"];
                            etags[after.get<string>()] = r->get_header_value("etag");
                            after = data["cursors"]["after"];

                            const auto &artists = data["items"].get<ArtistsT>();
                            result.insert(result.end(), artists.begin(), artists.end());
                            offset += artists.size();
                        }
                    }
                }
                while (!after.is_null());

                followed_artists.set(result);
                followed_artists_etags.set(etags);

                json request_url = httplib::append_query_params("/v1/me/tracks", {
                    { "limit", std::to_string(50) },
                    { "offset", std::to_string(2000) },
                });
                do
                {
                    if (auto r = api->get(request_url))
                    {
                        json data = json::parse(r->body);
                        request_url = data["next"];
                    }
                    else
                    {
                        request_url = "";
                    }
                }
                while (!request_url.is_null());
            }
        }
    }
}