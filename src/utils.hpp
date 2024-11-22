#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include <plugin.hpp>
#include <string>
#include <format>

namespace spotifar
{
	namespace utils
	{
		std::wstring get_plugin_launch_folder(const struct PluginStartupInfo* psInfo);

		std::string generate_random_string(const int);

		std::wstring to_wstring(const std::string& s);
	}
}


#endif //UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
