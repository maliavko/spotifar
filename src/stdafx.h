#ifndef STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#define STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
#pragma once

#define UNICODE
#define SPDLOG_WCHAR_FILENAMES
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <iostream>
#include <future>
#define _WINSOCKAPI_
#include <windows.h>
#include <shellapi.h>

#include "httplib.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "lng.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "guid.hpp"
#include "version.hpp"
#include "ObserverManager.h"

#endif //STDAFX_H_1C7F58A6_19FE_42B6_BD4D_300A869FD713
