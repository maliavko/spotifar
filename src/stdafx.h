#ifndef STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#define STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#pragma once

#define UNICODE
#define SPDLOG_WCHAR_FILENAMES
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define BS_THREAD_POOL_ENABLE_PRIORITY

#include <string>
#include <map>
#include <vector>
#include <list>
#include <chrono>
#define _WINSOCKAPI_
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <generator>
#include <windows.h>
#include <shellapi.h>  // for ShellExecute
#include <shlobj_core.h>  // for SHGetKnownFolderPath

#include "httplib.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "BS_thread_pool.hpp"
#include "ObserverManager.h"

#include <plugin.hpp>
#include <PluginSettings.hpp>
#include <DlgBuilder.hpp>

#include "lng.hpp"
#include "guid.hpp"
#include "version.hpp"

namespace spotifar
{
    using std::string;
    using std::wstring;
    using json = nlohmann::json;
}

#endif //STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
