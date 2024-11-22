#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "nlohmann\json.hpp"
#include <string>
#include <map>

namespace spotifar
{
	using std::map;
	using std::string;
	using json = nlohmann::json;

	struct Artist
	{
		string id;
		string name;
		size_t popularity;
	};

	typedef map<string, Artist> ArtistsCollection;

	void to_json(json& j, const Artist& a);

	void from_json(const json& j, Artist& a);
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641