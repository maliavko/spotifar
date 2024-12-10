#include "items.hpp" 

namespace spotifar
{
	namespace spotify
	{
		bool operator==(const Device& lhs, const Device& rhs)
		{
			return lhs.id == rhs.id;
		}
		
		void from_json(const json& j, Device& d)
		{
			j.at("id").get_to(d.id);
			j.at("is_active").get_to(d.is_active);
			j.at("name").get_to(d.name);
			j.at("type").get_to(d.type);
			j.at("volume_percent").get_to(d.volume_percent);
			j.at("supports_volume").get_to(d.supports_volume);

			d.user_name = utils::to_wstring(d.name);
		}
		
		void from_json(const json& j, PlaybackState& p)
		{
			j.at("device").get_to(p.device);
			j.at("repeat_state").get_to(p.repeat_state);
			j.at("shuffle_state").get_to(p.shuffle_state);
			j.at("progress_ms").get_to(p.progress_ms);
			j.at("is_playing").get_to(p.is_playing);
			j.at("actions").get_to(p.actions);

			if (j.contains("context") && !j["context"].is_null())
				j.at("context").get_to(*p.context);

			if (j.contains("track") && !j["track"].is_null())
				j.at("track").get_to(*p.track);
		}
	}
}