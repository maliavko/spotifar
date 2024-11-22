#include "stdafx.h"
#include "utils.hpp"

#include <filesystem>

namespace spotifar
{
	namespace utils
	{
		std::wstring get_plugin_launch_folder(const struct PluginStartupInfo* info)
		{
			return std::filesystem::path(info->ModuleName).parent_path().wstring();
		}

		std::string generate_random_string(const int length)
		{
			std::string text = "";
			const std::string possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

			for (int i = 0; i < length; i++)
			{
				float rand = (float)std::rand() / RAND_MAX * possible.length();
				text += possible[(int)std::floor(rand)];
			}
			return text;
		};

		std::wstring to_wstring(const std::string& s)
		{
			return std::wstring(s.begin(), s.end());
		}
	}
}
