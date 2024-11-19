#include "items.hpp" 

namespace spotifar
{
    void to_json(json& j, const Artist& p) {
		j = json{ {"name", p.name}, {"popularity", p.popularity} };
	}

	void from_json(const json& j, Artist& p) {
		j.at("name").get_to(p.name);
		j.at("popularity").get_to(p.popularity);
	}
}