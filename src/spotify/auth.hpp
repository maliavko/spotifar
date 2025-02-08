#ifndef AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#define AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#pragma once

#include "cache.hpp"

namespace spotifar { namespace spotify {

struct auth
{
    string access_token;
    string scope;
    int expires_in;
    string refresh_token;
    
    friend void from_json(const json &j, auth &a);
    friend void to_json(json &j, const auth &a);
};

class auth_cache: public json_cache<auth>
{
public:
    auth_cache(api_abstract *api, const string &client_id, const string &client_secret, int port);
    virtual ~auth_cache() { api = nullptr; }
    bool is_authenticated() const { return is_logged_in; }
protected:
    virtual bool request_data(auth &data);
    virtual clock_t::duration get_sync_interval() const;
    virtual void on_data_synced(const auth &data, const auth &prev_data);
    
    string request_auth_code();
    auth auth_with_code(const string &auth_code);
    auth auth_with_refresh_token(const string &refresh_token);
    auth authenticate(const httplib::Params &params);
    string get_auth_callback_url() const;

private:
    bool is_logged_in = false;
    const string client_id, client_secret;
    int port;
    
    api_abstract *api;
};

} // namespace spotify
} // namespace spotifar

#endif //AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA