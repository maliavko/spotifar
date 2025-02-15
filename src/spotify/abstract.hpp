#ifndef ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6
#define ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6

#include "utils.hpp"
#include "config.hpp"

namespace spotifar { namespace spotify {

struct api_abstract
{
    virtual ~api_abstract() {}
    
    /// @brief Sets the given `token` as a bearer token to the http client
    virtual void set_bearer_token_auth(const string &token) = 0;

    /// @brief Checks the spotify authorizations status
    virtual bool is_authenticated() const = 0;

    /// @brief Returns whether a playback is active. For sucn case the API
    /// syncs with the Spotify server happens way more often
    virtual bool is_playback_active() const = 0;

    /// @brief Performs a HTTP GET request
    /// @param cache_for caches the requested data for the givem amount of time
    virtual httplib::Result get(const string &request_url, utils::clock_t::duration cache_for = {}) = 0;

    virtual httplib::Result put(const string &request_url, const json &body = {}) = 0;

    virtual httplib::Result del(const string &request_url, const json &body = {}) = 0;
};

/// @brief An interface to the class, which implements the functionality to cache the data
/// and store it in the local storage
struct cached_data_abstract: public config::persistent_data_abstract
{
    /// @brief An method to resync data from the server
    /// @param force - if true, the data will be resynced regardless of the cache validity
    virtual void resync(bool force = false) = 0;

    /// @brief Return true if the cache should not be resynced
    virtual bool is_active() const { return true; }
};

} // namespace spotify
} // namespace spotifar

#endif // ABSTRACT_HPP_981E825E_A57D_4FB6_AA7D_FC27D37304A6