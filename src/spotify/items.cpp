#include "items.hpp" 

namespace spotifar
{
	namespace spotify
	{
		void from_json(const json& j, Artist& a) {
			j.at("id").get_to(a.id);
			j.at("name").get_to(a.name);
			j.at("popularity").get_to(a.popularity);
		}

		void from_json(const json& j, Album& a) {
			j.at("id").get_to(a.id);
			j.at("name").get_to(a.name);
		}

		void from_json(const json& j, Track& a) {
			j.at("id").get_to(a.id);
			j.at("name").get_to(a.name);
			j.at("track_number").get_to(a.track_number);
		}
	}
}