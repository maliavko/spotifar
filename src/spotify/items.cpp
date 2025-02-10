#include "items.hpp"
#include "utils.hpp"

namespace spotifar { namespace spotify {

string make_item_uri(const string &item_type_name, const string &id)
{
    return std::format("spotify:{}:{}", item_type_name, id);
}

void from_json(const json &j, simplified_artist &a)
{
    j.at("id").get_to(a.id);

    a.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const simplified_artist &a)
{
    j = json{
        { "id", a.id },
        { "name", utils::utf8_encode(a.name) },
    };
}

void from_json(const json &j, artist &a)
{
    from_json(j, dynamic_cast<simplified_artist&>(a));

    j.at("popularity").get_to(a.popularity);
}

void to_json(json &j, const artist &a)
{
    to_json(j, dynamic_cast<const simplified_artist&>(a));

    j.update({
        { "popularity", a.popularity },
    });
}

string simplified_album::get_release_year() const
{
    static auto r = std::regex("[\\d]{4}");	
    std::smatch match;
    if (std::regex_search(release_date, match, r))
        return match[0];
    return "----";
}

wstring simplified_album::get_user_name() const
{
    wstring user_name = std::format(L"[{}] {}", utils::to_wstring(get_release_year()), name);
    if (is_single())
        user_name += L" [EP]";
    return user_name;
}

void from_json(const json &j, simplified_album &a)
{
    j.at("id").get_to(a.id);
    j.at("total_tracks").get_to(a.total_tracks);
    j.at("album_type").get_to(a.album_type);
    j.at("release_date").get_to(a.release_date);

    a.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const simplified_album &a)
{
    j = json{
        { "id", a.id },
        { "total_tracks", a.total_tracks },
        { "album_type", a.album_type },
        { "release_date", a.release_date },
        { "name", utils::utf8_encode(a.name) },
    };
}

void from_json(const json &j, album &a)
{
    from_json(j, dynamic_cast<simplified_album&>(a));
}

void to_json(json &j, const album &a)
{
    to_json(j, dynamic_cast<const simplified_album&>(a));
    //j.update
}

const string& SimplifiedTrack::get_fields_filter()
{
    static string fields = "id,name,duration_ms,track_number";
    return fields;
}

bool operator==(const SimplifiedTrack &lhs, const SimplifiedTrack &rhs)
{
    return lhs.id == rhs.id;
}

void from_json(const json &j, SimplifiedTrack &t)
{
    j.at("id").get_to(t.id);
    j.at("track_number").get_to(t.track_number);
    j.at("duration_ms").get_to(t.duration_ms);

    t.duration = t.duration_ms / 1000;
    t.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const SimplifiedTrack &t)
{
    j = json{
        { "id", t.id },
        { "duration_ms", t.duration_ms },
        { "track_number", t.track_number },
        { "name", utils::utf8_encode(t.name) },
    };
}

const string& Track::get_fields_filter()
{
    static string fields = std::format("{},album,artists", SimplifiedTrack::get_fields_filter());
    return fields;
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

void from_json(const json &j, SimplifiedPlaylist &p)
{
    j.at("id").get_to(p.id);
    j.at("tracks").at("total").get_to(p.tracks_total);
    
    p.name = utils::utf8_decode(j.at("name").get<string>());
    p.description = utils::utf8_decode(j.at("description").get<string>());
}

const string& PlaylistTrack::get_fields_filter()
{
    static string fields = std::format("added_at,track({})", Track::get_fields_filter());
    return fields;
}

} // namespace spotify
} // namespace spotifar