#include "items.hpp"
#include "utils.hpp"

// TODO: after I started saving response strings instead of packed json, I think most of the to_json functions are not needed, review

namespace spotifar { namespace spotify {

string make_item_uri(const string &item_type_name, const string &id)
{
    return std::format("spotify:{}:{}", item_type_name, id);
}

bool operator==(const data_item_t &lhs, const data_item_t &rhs)
{
    return lhs.id == rhs.id;
}

void from_rapidjson(const rapidjson::Value &j, string &result)
{
    result = j.GetString();
}

void from_rapidjson(const rapidjson::Value &j, image_t &i)
{
    i.url = j["url"].GetString();
    i.width = j["width"].GetUint64();
    i.height = j["height"].GetUint64();
}

// void to_json(json &j, const simplified_artist_t &a)
// {
//     j = json{
//         { "id", a.id },
//         { "name", utils::utf8_encode(a.name) },
//     };
// }

void from_rapidjson(const rapidjson::Value &j, simplified_artist_t &a)
{
    a.id = j["id"].GetString();
    a.name = utils::utf8_decode(j["name"].GetString());
}

// void to_json(json &j, const artist_t &a)
// {
//     to_json(j, dynamic_cast<const simplified_artist_t&>(a));

//     j.update({
//         { "popularity", a.popularity },
//         { "genres", a.genres },
//         { "images", a.images },
//         { "followers", { "total", a.followers_total } },
//     });
// }

void from_rapidjson(const rapidjson::Value &j, artist_t &a)
{
    from_rapidjson(j, dynamic_cast<simplified_artist_t&>(a));

    a.popularity = j["popularity"].GetUint64();
    a.followers_total = j["followers"]["total"].GetUint64();
    
    from_rapidjson(j["images"], a.images);
    from_rapidjson(j["genres"], a.genres);
}

string simplified_album_t::get_release_year() const
{
    static auto r = std::regex("[\\d]{4}");	
    std::smatch match;
    if (std::regex_search(release_date, match, r))
        return match[0];
    return "----";
}

wstring simplified_album_t::get_type_abbrev() const
{
    if (album_type == album)
        return L"LP";
    if (album_type == single)
        return L"EP";
    if (album_type == compilation)
        return L"COMP";
    return L"??";
}

wstring simplified_album_t::get_user_name() const
{
    wstring user_name = std::format(L"[{}] {}", utils::to_wstring(get_release_year()), name);
    if (is_single())
        user_name += L" [EP]";
    return user_name;
}

void from_rapidjson(const rapidjson::Value &j, simplified_album_t &a)
{
    a.id = j["id"].GetString();
    a.name = utils::utf8_decode(j["name"].GetString());
    a.total_tracks = j["total_tracks"].GetUint64();
    a.album_type = j["album_type"].GetString();
    a.release_date = j["release_date"].GetString();
    a.href = j["href"].GetString();

    from_rapidjson(j["images"], a.images);
    from_rapidjson(j["artists"], a.artists);
}

// void to_json(json &j, const simplified_album_t &a)
// {
//     j = json{
//         { "id", a.id },
//         { "total_tracks", a.total_tracks },
//         { "album_type", a.album_type },
//         { "release_date", a.release_date },
//         { "href", a.href },
//         { "images", a.images },
//         { "artists", a.artists },
//         { "name", utils::utf8_encode(a.name) },
//     };
// }

// void to_json(json &j, const album_t &a)
// {
//     to_json(j, dynamic_cast<const simplified_album_t&>(a));
//     //j.update
// }

void from_rapidjson(const rapidjson::Value &j, album_t &a)
{
    from_rapidjson(j, dynamic_cast<simplified_album_t&>(a));
}

// void to_json(json &j, const saved_album_t &a)
// {
//     j = json{
//         { "added_at", a.added_at },
//         { "album", dynamic_cast<const album_t&>(a) },
//     };
// }

void from_rapidjson(const rapidjson::Value &j, saved_album_t &a)
{
    from_rapidjson(j["album"], dynamic_cast<album_t&>(a));
    a.added_at = j["added_at"].GetString();
}

const string& simplified_track_t::get_fields_filter()
{
    static string fields = "id,name,duration_ms,disc_number,track_number,explicit,artists";
    return fields;
}

// void to_json(json &j, const simplified_track_t &t)
// {
//     j = json{
//         { "id", t.id },
//         { "duration_ms", t.duration_ms },
//         { "track_number", t.track_number },
//         { "disc_number", t.disc_number },
//         { "explicit", t.is_explicit },
//         { "artists", t.artists },
//         { "name", utils::utf8_encode(t.name) },
//     };
// }

void from_rapidjson(const rapidjson::Value &j, simplified_track_t &t)
{
    t.id = j["id"].GetString();
    t.track_number = j["track_number"].GetUint64();
    t.disc_number = j["disc_number"].GetUint64();
    t.duration_ms = j["duration_ms"].GetInt();
    t.is_explicit = j["explicit"].GetBool();
    
    t.duration = t.duration_ms / 1000;
    t.name = utils::utf8_decode(j["name"].GetString());
}

const string& track_t::get_fields_filter()
{
    static string fields = std::format("{},album,artists", simplified_track_t::get_fields_filter());
    return fields;
}

wstring track_t::get_artists_full_name() const
{
    std::vector<wstring> artists_names;
    std::transform(artists.cbegin(), artists.cend(), back_inserter(artists_names),
        [](const auto &a) { return a.name; });
    return utils::string_join(artists_names, L", ");
}

wstring track_t::get_artist_name() const
{
    if (artists.size() > 0)
        return artists[0].name;
    return L"Unknown"; // TODO: localize
}

wstring track_t::get_long_name() const
{
    return std::format(L"{} - {}", get_artist_name(), name);
}

// void to_json(json &j, const track_t &t)
// {
//     to_json(j, dynamic_cast<const simplified_track_t&>(t));

//     j.update({
//         {"album", t.album},
//         {"artists", t.artists},
//     });
// }

void from_rapidjson(const rapidjson::Value &j, track_t &t)
{
    from_rapidjson(j, dynamic_cast<simplified_track_t&>(t));

    from_rapidjson(j["album"], t.album);
    from_rapidjson(j["artists"], t.artists);
}

const string& saved_track_t::get_fields_filter()
{
    static string fields = std::format("added_at,track({})", track_t::get_fields_filter());
    return fields;
}

// void to_json(json &j, const saved_track_t &t)
// {
//     j = json{
//         { "added_at", t.added_at },
//         { "track", dynamic_cast<const track_t&>(t) },
//     };
// }
    
void from_rapidjson(const rapidjson::Value &j, saved_track_t &t)
{
    from_rapidjson(j["track"], dynamic_cast<track_t&>(t));
    t.added_at = j["added_at"].GetString();
}

// void to_json(json &j, const simplified_playlist_t &p)
// {
//     j = {
//         { "id", p.id },
//         { "snapshot_id", p.snapshot_id },
//         { "href", p.href },
//         { "collaborative", p.collaborative },
//         { "public", p.is_public },
//         { "tracks", {
//             { "total", p.tracks_total }
//         } },
//         { "name", utils::utf8_encode(p.name) },
//         { "description", utils::utf8_encode(p.description) },
//         { "owner", {
//             { "display_name", utils::utf8_encode(p.user_display_name)}
//         } },
//     };
// }

void from_rapidjson(const rapidjson::Value &j, simplified_playlist_t &p)
{
    p.id = j["id"].GetString();
    p.snapshot_id = j["snapshot_id"].GetString();
    p.href = j["href"].GetString();
    p.collaborative = j["collaborative"].GetBool();
    p.is_public = j["public"].GetBool();
    p.tracks_total = j["tracks"]["total"].GetUint64();
    
    p.name = utils::utf8_decode(j["name"].GetString());
    p.user_display_name = utils::utf8_decode(j["owner"]["display_name"].GetString());
    p.description = utils::utf8_decode(j["description"].GetString());
}

// void to_json(json &j, const playlist_t &p)
// {
//     to_json(j, dynamic_cast<const simplified_playlist_t&>(p));
// }

void from_rapidjson(const rapidjson::Value &j, playlist_t &p)
{
    from_rapidjson(j, dynamic_cast<simplified_playlist_t&>(p));
}

const string& simplified_playlist_t::get_fields_filter()
{
    static string fields = std::format("id,href,name,collaborative,public,description,"
        "tracks(total),owner(display_name),snapshot_id");
    return fields;
}

const string& playlist_t::get_fields_filter()
{
    static string fields = std::format("{}", simplified_playlist_t::get_fields_filter());
    return fields;
}

void to_json(json &j, const playback_state_t &p)
{
    j = json{
        // { "device", p.device },
        // { "repeat_state", p.repeat_state },
        // { "shuffle_state", p.shuffle_state },
        // { "progress_ms", p.progress_ms },
        // { "is_playing", p.is_playing },
        // { "actions", p.actions },
        // { "item", p.item },
        // { "context", p.context },
    };
}

void from_rapidjson(const rapidjson::Value &j, playback_state_t &p)
{
    from_rapidjson(j["device"], p.device);
    from_rapidjson(j["actions"], p.actions);

    if (j.HasMember("context") && !j["context"].IsNull())
        from_rapidjson(j["context"], p.context);

    if (j.HasMember("item") && !j["item"].IsNull())
        from_rapidjson(j["item"], p.item);

    p.progress_ms = j.HasMember("progress_ms") ? j["progress_ms"].GetInt() : 0;
    p.progress = p.progress_ms / 1000;

    p.repeat_state = j["repeat_state"].GetString();
    p.shuffle_state = j["shuffle_state"].GetBool();
    p.is_playing = j["is_playing"].GetBool();
}

// void to_json(json &j, const playing_queue_t &p)
// {
//     j = json{
//         { "queue", p.queue },
//         { "currently_playing", p.currently_playing },
//     };
// }

bool operator==(const actions_t &lhs, const actions_t &rhs)
{
    return (
        lhs.interrupting_playback == rhs.interrupting_playback &&
        lhs.pausing == rhs.pausing &&
        lhs.resuming == rhs.resuming &&
        lhs.seeking == rhs.seeking &&
        lhs.skipping_next == rhs.skipping_next &&
        lhs.skipping_prev == rhs.skipping_prev &&
        lhs.toggling_repeat_context == rhs.toggling_repeat_context &&
        lhs.toggling_repeat_track == rhs.toggling_repeat_track &&
        lhs.toggling_shuffle == rhs.toggling_shuffle &&
        lhs.trasferring_playback == rhs.trasferring_playback
    );
}

void from_rapidjson(const rapidjson::Value &j, actions_t &a)
{
    auto get_bool = [&](const string &fieldname) -> bool
    {
        return j.HasMember(fieldname) ? j[fieldname].GetBool() : false;
    };

    a.interrupting_playback = get_bool("interrupting_playback");
    a.pausing = get_bool("pausing");
    a.resuming = get_bool("resuming");
    a.seeking = get_bool("seeking");
    a.skipping_next = get_bool("skipping_next");
    a.skipping_prev = get_bool("skipping_prev");
    a.toggling_repeat_context = get_bool("toggling_repeat_context");
    a.toggling_repeat_track = get_bool("toggling_repeat_track");
    a.toggling_shuffle = get_bool("toggling_shuffle");
    a.trasferring_playback = get_bool("trasferring_playback");
}

void from_rapidjson(const rapidjson::Value &j, context_t &c)
{
    c.type = j["type"].GetString();
    c.uri = j["uri"].GetString();
    c.href = j["href"].GetString();
}

// void to_json(json &j, const actions_t &p)
// {
//     // TODO: unfinished
//     j = json{};
// }

string context_t::get_item_id() const
{
    static auto r = std::regex("(\\w+):(\\w+):(\\w+)");	
    std::smatch match;
    if (std::regex_search(uri, match, r))
        return match[3];
    return "";
}

bool operator==(const context_t &lhs, const context_t &rhs)
{
    return lhs.href == rhs.href;
}

void to_json(json &j, const device_t &d)
{
    j = json{
        { "id", d.id },
        { "is_active", d.is_active },
        { "name", utils::utf8_encode(d.name) },
        { "type", d.type },
        { "volume_percent", d.volume_percent },
        { "supports_volume", d.supports_volume },
    };
}

void from_rapidjson(const rapidjson::Value &j, device_t &d)
{
    d.id = j["id"].GetString();
    d.is_active = j["is_active"].GetBool();
    d.type = j["type"].GetString();
    d.supports_volume = j["supports_volume"].GetBool();
    d.name = utils::utf8_decode(j["name"].GetString());

    d.volume_percent = j.HasMember("volume_percent") ? j["volume_percent"].GetInt() : 0;
}

string device_t::to_str() const
{
    return std::format("device(name={}, id={})", utils::to_string(name), id);
}

void to_json(json &j, const history_item_t &p)
{
    j = json{
        {"played_at", p.played_at},
        // {"context", p.context},
        // {"track", p.track},
        // TODO: rapidjson, write a rapidjson_to method and check if recently playing works well
    };
}

    
void from_rapidjson(const rapidjson::Value &j, history_item_t &p)
{
    p.played_at = j["played_at"].GetString();
    from_rapidjson(j["track"], p.track);

    if (!j["context"].IsNull())
        from_rapidjson(j["context"], p.context);
}

} // namespace spotify
} // namespace spotifar