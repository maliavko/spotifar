#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641

#pragma once

#include <string>
#include "nlohmann\json.hpp"

namespace spotifar
{
	using std::string;
	using json = nlohmann::json;

	struct Artist
	{
		string name;
		size_t popularity;
	};

	void to_json(json& j, const Artist& p);

	void from_json(const json& j, Artist& p);
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641