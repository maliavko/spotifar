#include "stdafx.h"

namespace spotifar
{
	namespace utils
	{
		string generate_random_string(const int length)
		{
			string text = "";
			const string possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

			for (int i = 0; i < length; i++)
			{
				float rand = (float)std::rand() / RAND_MAX * possible.length();
				text += possible[(int)std::floor(rand)];
			}
			return text;
		};

		template<>
		string InputParser::get_cmd_option(const string& option) const
		{
			std::vector<string>::const_iterator itr;
			itr = std::find(this->tokens.begin(), this->tokens.end(), option);
			if (itr != this->tokens.end() && ++itr != this->tokens.end())
			{
				return *itr;
			}

			throw ConfigException("No value found with the given name, " + option);
		}

		template<>
		int InputParser::get_cmd_option(const string& option) const
		{
			return std::stoi(get_cmd_option<string>(option));
		}
	}
}
