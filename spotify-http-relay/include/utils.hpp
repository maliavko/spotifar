#ifndef UTILS_HPP_EE26373A_3E77_41DB_A85C_B3AB65E3CCCD
#define UTILS_HPP_EE26373A_3E77_41DB_A85C_B3AB65E3CCCD
#pragma once

#include <string>
#include <vector>
#include <ctime>

#include "httplib.h"

namespace spotifar
{
	namespace utils
	{
		using std::string;

		string generate_random_string(const int);

		class ConfigException: public std::exception
		{
		public:
			explicit ConfigException(const char* message)
				: std::exception(message)
			{}

			explicit ConfigException(const string& message)
				: std::exception(message.c_str())
			{}
		};

		// taken from StackOverflow
		// https://stackoverflow.com/questions/865668/parsing-command-line-arguments-in-c?page=1&tab=scoredesc#tab-top
		class InputParser
		{
		public:
			InputParser(int& argc, char** argv)
			{
				for (int i = 1; i < argc; ++i)
					this->tokens.push_back(string(argv[i]));
			}
			
			template<typename ReturnType>
			ReturnType get_cmd_option(const string& option) const;
		private:
			std::vector <string> tokens;
		};
	}
}

#endif //UTILS_HPP_EE26373A_3E77_41DB_A85C_B3AB65E3CCCD
