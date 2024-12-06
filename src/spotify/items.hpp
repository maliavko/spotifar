#ifndef ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#define ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641
#pragma once

#include "nlohmann\json.hpp"
#include <string>
#include <map>

namespace spotifar
{
	namespace spotify
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
		void from_json(const json& j, Artist& a);

		struct Album
		{
			string id;
			string name;
		};
		typedef map<string, Album> AlbumsCollection;
		void from_json(const json& j, Album& a);

		struct Track
		{
			string id;
			string name;
			size_t track_number;  // TODO: track number could be duplicated for different discs
		};
		typedef map<string, Track> TracksCollection;
		void from_json(const json& j, Track& a);

		struct PlaybackInfo
		{

		};
	}
}

#endif //ITEMS_HPP_55A04E12_800F_4468_BD38_54D0CC81EF641