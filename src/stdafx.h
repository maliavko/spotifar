#ifndef STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#define STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#pragma once

#define UNICODE
#define SPDLOG_WCHAR_FILENAMES // using wstring with spdlog library
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define BS_THREAD_POOL_ENABLE_PRIORITY // possibility to use priorities for the thread tasks in BS::thread_pool
#define RAPIDJSON_HAS_STDSTRING 1 // using string with rapidjson library
// replacing asserts with runtime exceptions, while working with rapidson for proper error handling
#define RAPIDJSON_ASSERT(x) if (!(x)) throw std::runtime_error("json error, " #x);

#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <chrono> // std::chrono::system_clock
#include <typeindex> // std::type_index
#include <filesystem> // std::filesystem::path
#define _WINSOCKAPI_
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <generator> // std::generator<T>
#include <windows.h> // win api support
#include <shellapi.h>  // for ShellExecute
#include <shlobj_core.h>  // for SHGetKnownFolderPath

#include "httplib.h" // single-threaded http client/server library
#include "spdlog/spdlog.h" // logging library
#include "BS_thread_pool.hpp" // thread pool library
#include "ObserverManager.h" // event bus library
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "wintoastlib.h" // win toast notifications library

#include <plugin.hpp> // far api
#include <PluginSettings.hpp> // far plugin's data storage access
#include <DlgBuilder.hpp> // far automatic dialogs builders

#include "guid.hpp"
#include "version.hpp"

namespace spotifar
{
    using std::string;
    using std::wstring;
    using namespace std::literals;
}

#endif //STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
