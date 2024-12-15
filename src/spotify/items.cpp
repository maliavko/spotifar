#include "items.hpp" 

namespace spotifar
{
	namespace spotify
	{
		std::string SimplifiedArtist::to_str() const
		{
			return std::format("SimplifiedArtist(name={}, id={})", name, id);
		}

		std::string Album::to_str() const
		{
			return std::format("Album(name={}, id={})", name, id);
		}

		std::string Device::to_str() const
		{
			return std::format("Device(name={}, id={})", name, id);
		}

		bool operator==(const Device &lhs, const Device &rhs)
		{
			return lhs.id == rhs.id;
		}

		void from_json(const json& j, Device& d)
		{
			j.at("id").get_to(d.id);
			j.at("is_active").get_to(d.is_active);
			j.at("name").get_to(d.name);
			j.at("type").get_to(d.type);
			j.at("supports_volume").get_to(d.supports_volume);

			d.volume_percent = j.value("volume_percent", 100);
			d.user_name = utils::to_wstring(d.name);
		}
		
		void from_json(const json& j, PlaybackState& p)
		{
			j.at("device").get_to(p.device);
			j.at("repeat_state").get_to(p.repeat_state);
			j.at("shuffle_state").get_to(p.shuffle_state);
			p.progress_ms = j.value("progress_ms", 0);
			j.at("is_playing").get_to(p.is_playing);
			j.at("actions").get_to(p.permissions);

			if (j.contains("context") && !j.at("context").is_null())
			{
				p.context = std::make_shared<Context>();
				j.at("context").get_to(*p.context);
			}

			if (j.contains("item") && !j.at("item").is_null())
			{
				p.track = std::make_shared<Track>();
				j.at("item").get_to(*p.track);
			}
		}
		
		void from_json(const json& j, Permissions& p)
		{
			p.interrupting_playback = j.value("interrupting_playback", false);
			p.pausing = j.value("pausing", false);
			p.resuming = j.value("resuming", false);
			p.seeking = j.value("seeking", false);
			p.skipping_next = j.value("skipping_next", false);
			p.skipping_prev = j.value("skipping_prev", false);
			p.toggling_repeat_context = j.value("toggling_repeat_context", false);
			p.toggling_repeat_track = j.value("toggling_repeat_track", false);
			p.toggling_shuffle = j.value("toggling_shuffle", false);
			p.trasferring_playback = j.value("trasferring_playback", false);
		}
	}
}