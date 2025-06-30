#include "spotify/requesters.hpp"

namespace spotifar { namespace spotify {

using namespace httplib;
using namespace utils;

const size_t max_limit = 50ULL;

void http_logger(const Request &req, const Response &res)
{
    static const std::set<string> exclude{
        //"/v1/me/player",
        //"/v1/me/player/devices",
        //"/v1/me/player/recently-played",
    };
    
    if (utils::http::is_success(res.status))
    {
        if (!exclude.contains(http::trim_params(req.path)))
        {
            log::api->debug("A successful HTTP request (code={}): [{}] {} {}",
                res.status, req.method, http::trim_domain(req.path), req.body);
        }
    }
    else
    {
        log::api->error(http::dump_error(req, res));
    }
}

bool modify_requester::execute(api_weak_ptr_t api_proxy)
{
    if (auto api = api_proxy.lock())
    {
        response = execute_request(api.get());
        if (!utils::http::is_success(response))
        {
            log::api->error("There is an error while executing API modify request: '{}', "
                "url '{}', body '{}'", utils::http::get_status_message(response), url, body);
            return false;
        }
    }
    return true;
}

httplib::Result put_requester::execute_request(api_interface *api)
{
    return api->put(get_url(), get_body());
}

httplib::Result del_requester::execute_request(api_interface *api)
{
    return api->del(get_url(), get_body());
}

} // namespace spotify
} // namespace spotifar