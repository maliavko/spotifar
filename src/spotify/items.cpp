#include "items.hpp"

namespace spotifar
{
	namespace spotify
	{
		std::string SimplifiedArtist::to_str() const
		{
			return std::format("SimplifiedArtist(name={}, id={})", utils::to_string(name), id);
		}

		std::string Album::to_str() const
		{
			return std::format("Album(name={}, id={})", utils::to_string(name), id);
		}

		std::string Device::to_str() const
		{
			return std::format("Device(name={}, id={})", utils::to_string(name), id);
		}

		bool operator==(const Device &lhs, const Device &rhs)
		{
			return lhs.id == rhs.id;
		}

		void from_json(const json &j, SimplifiedArtist &a)
		{
			j.at("id").get_to(a.id);

			a.name = utils::utf8_decode(j.at("name").get<string>());
		}
		
		void from_json(const json &j, Artist &a)
		{
			from_json(j, dynamic_cast<SimplifiedArtist&>(a));

			j.at("popularity").get_to(a.popularity);
		}

		void from_json(const json &j, Album &t)
		{
			j.at("id").get_to(t.id);

			t.name = utils::utf8_decode(j.at("name").get<string>());
		}

		void from_json(const json &j, SimplifiedTrack &t)
		{
			j.at("id").get_to(t.id);
			j.at("track_number").get_to(t.track_number);

			t.name = utils::utf8_decode(j.at("name").get<string>());
		}
		
		void from_json(const json &j, Track &t)
		{
			from_json(j, dynamic_cast<SimplifiedTrack&>(t));
			try{
			j.at("album").get_to(t.album);
			j.at("artists").get_to(t.artists);
			j.at("duration_ms").get_to(t.duration_ms);
			} catch (std::exception& e)
			{
				auto s = e.what();
				int i = 0;
			}
		}

		void from_json(const json &j, Device &d)
		{
			j.at("id").get_to(d.id);
			j.at("is_active").get_to(d.is_active);
			j.at("type").get_to(d.type);
			j.at("supports_volume").get_to(d.supports_volume);

			d.volume_percent = j.value("volume_percent", 100);
			d.name = utils::utf8_decode(j.at("name").get<string>());
		}
		
		void from_json(const json &j, PlaybackState &p)
		{
			j.at("device").get_to(p.device);
			j.at("repeat_state").get_to(p.repeat_state);
			j.at("shuffle_state").get_to(p.shuffle_state);
			j.at("is_playing").get_to(p.is_playing);
			j.at("actions").get_to(p.permissions);

			p.progress_ms = j.value("progress_ms", 0);

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
		
		void from_json(const json &j, Permissions &p)
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