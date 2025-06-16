#ifndef AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#define AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#pragma once

#include "interfaces.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;

class auth_cache: public json_cache<auth_t>
{
public:
    auth_cache(api_interface *api, const string &client_id, const string &client_secret, int port);
    ~auth_cache() { api_proxy = nullptr; }
    
    void shutdown(config::settings_context &ctx) override;

    bool is_authenticated() const { return is_logged_in; }
    auto get_access_token() const -> const string& { return get().access_token; }
protected:
    string request_auth_code();
    auth_t auth_with_code(const string &auth_code);
    auth_t auth_with_refresh_token(const string &refresh_token);
    auth_t authenticate(const httplib::Params &params);
    string get_auth_callback_url() const;
    
    // json_cache interface
    bool request_data(auth_t &data) override;
    auto get_sync_interval() const -> clock_t::duration override;
    void on_data_synced(const auth_t &data, const auth_t &prev_data) override;
private:
    bool is_logged_in = false;
    const string client_id;
    const string client_secret;
    int port;

    httplib::Server auth_server;
    api_interface *api_proxy;
};

} // namespace spotify
} // namespace spotifar

#endif //AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA