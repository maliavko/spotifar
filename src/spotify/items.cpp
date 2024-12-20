#include "items.hpp"

namespace spotifar
{
	namespace spotify
	{
		std::string SimplifiedArtist::to_str() const
		{
			return std::format("SimplifiedArtist(name={}, id={})", utils::to_string(name), id);
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

		void to_json(json &j, const SimplifiedArtist &p)
		{
			j = json{
				{ "id", p.id },
				{ "name", utils::utf8_encode(p.name) },
			};
		}
		
		void from_json(const json &j, Artist &a)
		{
			from_json(j, dynamic_cast<SimplifiedArtist&>(a));

			j.at("popularity").get_to(a.popularity);
		}

		void from_json(const json &j, SimplifiedAlbum &a)
		{
			j.at("id").get_to(a.id);
			j.at("total_tracks").get_to(a.total_tracks);
			j.at("album_type").get_to(a.album_type);
			//j.at("album_group").get_to(a.album_group);
			j.at("release_date").get_to(a.release_date);

			static auto r = std::regex("[\\d]{4}");	
			std::smatch match;
			if (std::regex_search(a.release_date, match, r))
				a.release_year = match[0];
			else
				a.release_year = "----";

			a.name = utils::utf8_decode(j.at("name").get<string>());
		}

		void to_json(json &j, const SimplifiedAlbum &a)
		{
			j = json{
				{ "id", a.id },
				{ "total_tracks", a.total_tracks },
				{ "album_type", a.album_type },
				{ "release_date", a.release_date },
				{ "name", utils::utf8_encode(a.name) },
			};
		}

		std::string SimplifiedAlbum::to_str() const
		{
			return std::format("SimplifiedAlbum(name={}, id={})", utils::to_string(name), id);
		}

		void from_json(const json &j, Album &a)
		{
			from_json(j, dynamic_cast<SimplifiedAlbum&>(a));
		}

		void to_json(json &j, const Album &a)
		{
			to_json(j, dynamic_cast<const SimplifiedAlbum&>(a));
			//j.update
		}

		void from_json(const json &j, SimplifiedTrack &t)
		{
			j.at("id").get_to(t.id);
			j.at("track_number").get_to(t.track_number);

			t.duration = (size_t)j.at("duration_ms").get<int>() / 1000;
			t.name = utils::utf8_decode(j.at("name").get<string>());
		}
		
		void to_json(json &j, const SimplifiedTrack &t)
		{
			j = json{
				{ "id", t.id },
				{ "duration_ms", t.duration * 1000 },
				{ "track_number", t.track_number },
				{ "name", utils::utf8_encode(t.name) },
			};
		}
		
		void from_json(const json &j, Track &t)
		{
			from_json(j, dynamic_cast<SimplifiedTrack&>(t));
			j.at("album").get_to(t.album);
			j.at("artists").get_to(t.artists);
		}
		
		void to_json(json &j, const Track &t)
		{
			to_json(j, dynamic_cast<const SimplifiedTrack&>(t));

			j.update({
				{"album", t.album},
				{"artists", t.artists},
			});
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
		
		void from_json(const json &j, PlaybackState &p)
		{
			j.at("device").get_to(p.device);
			j.at("repeat_state").get_to(p.repeat_state);
			j.at("shuffle_state").get_to(p.shuffle_state);
			j.at("is_playing").get_to(p.is_playing);
			j.at("actions").get_to(p.permissions);

			p.progress = j.value("progress_ms", 0) / 1000;

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
		
		void from_json(const json &j, SimplifiedPlaylist &p)
		{
			j.at("id").get_to(p.id);
			j.at("tracks").at("total").get_to(p.tracks_total);
			
			p.name = utils::utf8_decode(j.at("name").get<string>());
			p.description = utils::utf8_decode(j.at("description").get<string>());
		}
		
		void from_json(const json &j, HistoryItem &p)
		{
			j.at("played_at").get_to(p.played_at);
			j.at("context").get_to(p.context);
			j.at("track").get_to(p.track);
		}

		void to_json(json &j, const HistoryItem &p)
		{
			j = json{
				{"played_at", p.played_at},
				{"context", p.context},
				{"track", p.track},
			};
		}
	}
}