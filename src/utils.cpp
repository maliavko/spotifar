#include "stdafx.h"
#include "utils.hpp"

#include <filesystem>

namespace spotifar
{
	namespace utils
	{
		std::wstring get_plugin_launch_folder(const struct PluginStartupInfo* psInfo)
		{
			return std::filesystem::path(psInfo->ModuleName).parent_path().wstring();
		}
	}
}
