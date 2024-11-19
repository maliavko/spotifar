#include "items.hpp" 

namespace spotifar
{
    void to_json(json& j, const Artist& a) {
		j = json{
			{"name", a.name},
			{"popularity", a.popularity},
			{"id", a.id}
		};
	}

	void from_json(const json& j, Artist& a) {
		j.at("name").get_to(a.name);
		j.at("id").get_to(a.id);
		j.at("popularity").get_to(a.popularity);
	}
}