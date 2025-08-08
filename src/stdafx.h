#ifndef STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#define STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#pragma once

// replacing asserts with runtime exceptions, while working with rapidson for proper error handling
#define RAPIDJSON_ASSERT(x) if (!(x)) throw std::runtime_error("json error, " #x);

// MinGW does not define these keys
#if defined(__MINGW32__)
#   ifndef VK_NAVIGATION_UP
#       define VK_NAVIGATION_UP 0x8A
#   endif
#   ifndef VK_NAVIGATION_DOWN
#       define VK_NAVIGATION_DOWN 0x8B
#   endif
#endif

#ifndef NOMINMAX
# define NOMINMAX
#endif

#include <string>
// this <generator> header is quite a pain: it hits "winsock include" warning on MinGW
// builds all the times, plus it works only if you put it with some specific order of headers
#pragma clang diagnostic ignored "-W#warnings"
#include <generator>

#include <windows.h> // win api support
#include <fstream> // IWYU pragma: keep
#include <map> // IWYU pragma: keep
#include <vector>
#include <chrono> // std::chrono::system_clock
#include <typeindex> // IWYU pragma: keep; std::type_index
#include <filesystem> // IWYU pragma: keep; std::filesystem::path
#include <shellapi.h>  // for ShellExecute
#include <shlobj.h> // for SHGetKnownFolderPath

#if defined(__cpp_lib_format)
#   define USING_STD_FORMAT
#endif

// @note as of 8/8/2025 the situation is the following
//  - MSVC 19.44 & GCC 15.1 compiles well with both options
//  - Clang 20.1.8 compiles only with standard library
//  - github workflows only with fmt::format
#ifdef USING_STD_FORMAT
#   define SPDLOG_USE_STD_FORMAT
#   include <format>
#else
#   include <fmt/chrono.h>
#   include <fmt/xchar.h>
#endif
    
#pragma GCC diagnostic ignored "-Wcpp"
#include "httplib.h" // IWYU pragma: keep; single-threaded http client/server library
#include "spdlog/spdlog.h" // IWYU pragma: keep; logging library
#include "BS_thread_pool.hpp" // IWYU pragma: keep; thread pool library
#include "ObserverManager.h" // IWYU pragma: keep; event bus library
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

#if defined(WIN_TOAST_ENABLED)
#   include "wintoastlib.h" // win toast notifications library
#endif

#include <plugin.hpp> // Far api

#include "guid.hpp" // IWYU pragma: keep
#include "version.hpp" // IWYU pragma: keep

namespace spotifar
{
    using std::string;
    using std::wstring;
    using namespace std::literals;

    struct plugin_interface;

    class plugin;
    class librespot;
    class hotkeys_handler;
    
    using plugin_ptr_t = std::shared_ptr<plugin_interface>;
    using plugin_weak_ptr_t = std::weak_ptr<plugin_interface>;

    namespace spotify
    {
        // interfaces
        struct collection_interface;
        struct api_interface;
        struct library_interface;
        struct cached_data_abstract;

        // classes
        class http_cache;
        class library;
        class playback_cache;
        class devices_cache;
        class auth_cache;
        class play_history;
        class recent_releases;
        class api;

        struct data_item_t;
        struct image_t;
        struct external_urls_t;
        struct copyrights_t;
        struct simplified_artist_t;
        struct simplified_album_t;
        struct simplified_track_t;
        struct artist_t;
        struct track_t;
        struct saved_track_t;
        struct album_t;
        struct saved_album_t;
        struct simplified_playlist_t;
        struct playlist_t;
        struct actions_t;
        struct context_t;
        struct device_t;
        struct playback_state_t;
        struct history_item_t;
        struct playing_queue_t;
        struct auth_t;

        using api_ptr_t         = std::shared_ptr<api_interface>;
        using api_weak_ptr_t    = std::weak_ptr<api_interface>;
        using collection_ptr    = std::shared_ptr<collection_interface>;
        using library_ptr       = std::shared_ptr<library_interface>;
        using item_id_t         = string;
        using item_ids_t        = std::vector<item_id_t>;
    }

    namespace ui
    {
        class player;
        class notifications_handler;
        class panel;
        class view;
    }

    namespace utils
    {
        using clock_t = std::chrono::system_clock;

        namespace json
        {
            using rapidjson::Document;
            using rapidjson::Value;
            using rapidjson::StringBuffer;
            using rapidjson::Writer;
            using rapidjson::SizeType;
            using rapidjson::Pointer;
            using rapidjson::PrettyWriter;
            using rapidjson::kObjectType;
            using rapidjson::kArrayType;
            using rapidjson::ParseResult;
            using Allocator = typename Document::AllocatorType;
        }

#ifdef USING_STD_FORMAT
    using std::format;
    using std::vformat;
    using std::make_format_args;
    using std::make_wformat_args;
    using std::string_view;
    using std::wstring_view;
#else
    using fmt::format;
    using fmt::vformat;
    using fmt::make_format_args;
    using fmt::make_wformat_args;
    using fmt::string_view;
    using fmt::wstring_view;
#endif
    }
}

#endif //STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
