#ifndef AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#define AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#pragma once

#include "abstract.hpp"
#include "cache.hpp"

namespace spotifar { namespace spotify {

namespace json = utils::json;

struct auth_t
{
    string access_token;
    string scope;
    int expires_in;
    string refresh_token;
    
    bool is_valid() const { return !access_token.empty(); }
    
    friend void from_json(const json::Value &j, auth_t &a);
    friend void to_json(json::Value &j, const auth_t &a, json::Allocator &allocator);
};

class auth_cache: public json_cache<auth_t>
{
public:
    auth_cache(api_interface *api, const string &client_id, const string &client_secret, int port);
    ~auth_cache() { api_proxy = nullptr; }
    
    bool is_authenticated() const { return is_logged_in; }
    auto get_access_token() const { return get().access_token; }
protected:
    virtual bool request_data(auth_t &data);
    virtual clock_t::duration get_sync_interval() const;
    virtual void on_data_synced(const auth_t &data, const auth_t &prev_data);
    
    string request_auth_code();
    auth_t auth_with_code(const string &auth_code);
    auth_t auth_with_refresh_token(const string &refresh_token);
    auth_t authenticate(const httplib::Params &params);
    string get_auth_callback_url() const;

private:
    bool is_logged_in = false;
    const string client_id, client_secret;
    int port;
    
    api_interface *api_proxy;
};

struct auth_observer: public BaseObserverProtocol
{
    /// @brief An auth status has been changed
    virtual void on_auth_status_changed(const auth_t &auth) {}
};

} // namespace spotify
} // namespace spotifar

#endif //AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA