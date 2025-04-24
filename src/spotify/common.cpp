#include "spotify/common.hpp"

namespace spotifar { namespace spotify {

using namespace httplib;
using namespace utils;

void http_logger(const Request &req, const Response &res)
{
    static const std::set<string> exclude{
        "/v1/me/player",
        "/v1/me/player/devices",
        "/v1/me/player/recently-played",
    };

    if (res.status == OK_200 || res.status == NoContent_204 || res.status == NotModified_304)
    {
        if (!exclude.contains(http::trim_params(req.path)))
        {
            log::api->debug("A successful HTTP request has been performed (code={}): [{}] {}",
                res.status, req.method, http::trim_domain(req.path));
        }
    }
    else
    {
        log::api->error(http::dump_error(req, res));
    }
}

} // namespace spotify
} // namespace spotifar