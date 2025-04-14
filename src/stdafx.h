#ifndef STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#define STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#pragma once

#define UNICODE
#define SPDLOG_WCHAR_FILENAMES
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define BS_THREAD_POOL_ENABLE_PRIORITY
#define RAPIDJSON_HAS_STDSTRING 1

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

#include "httplib.h"
#include "spdlog/spdlog.h"
#include "BS_thread_pool.hpp"
#include "ObserverManager.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "wintoastlib.h"

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
