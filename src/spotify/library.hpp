#ifndef LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465
#define LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465
#pragma once

#include "stdafx.h"
#include "common.hpp"

namespace spotifar { namespace spotify {

using saved_status_t = std::unordered_map<item_id_t, bool>;

class library
{
public:
    library(api_interface *api);
    ~library();

    void request_saved_status(const item_ids_t &ids);
private:
    api_interface *api_proxy;
    BS::light_thread_pool pool;

    // each request is slowed down to avoid API spamming by putting a thread into sleep;
    // to make it controllable and avoid blociking of application during closing e.g.,
    // the sleeping is done via conditional variable
    std::condition_variable cv;
    std::mutex cv_m;
    bool stop_flag = false;
};

struct collection_observer: public BaseObserverProtocol
{
    virtual void on_saved_tracks_status_received(const saved_status_t &saved_status) {}
};

} // namespace spotify
} // namespace spotifar

#endif // LIBRARY_HPP_92081AAB_EE4E_40B2_814E_83B712132465