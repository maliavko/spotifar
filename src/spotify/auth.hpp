#ifndef AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#define AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA
#pragma once

#include "cached_value.hpp"
#include "items.hpp"

namespace spotifar
{
    namespace spotify
    {
        using std::string;
        
        class AuthCache: public CachedItem<Auth>
        {
        public:
            AuthCache(IApi *api, const string &client_id, const string &client_secret, int port);
            virtual ~AuthCache() { api = nullptr; }

        protected:
            virtual bool request_data(Auth &data);
            virtual utils::ms get_sync_interval() const;
            virtual void on_data_synced(const Auth &data, const Auth &prev_data);
            
            string request_auth_code();
            Auth auth_with_code(const string &auth_code);
            Auth auth_with_refresh_token(const string &refresh_token);
            Auth auth(const httplib::Params &params);
		    string get_auth_callback_url() const;

        private:
            const std::string client_id, client_secret;
            int port;
            
            std::shared_ptr<spdlog::logger> logger;
            IApi *api;
        };
    }
}

#endif //AUTH_HPP_EB78B9BD_C144_43F0_9A9F_EB678C5C23AA