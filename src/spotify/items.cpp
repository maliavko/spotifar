#include "items.hpp"
#include "utils.hpp"

// TODO: after I started saving response strings instead of packed json, I think most of the to_json functions are not needed, review

namespace spotifar { namespace spotify {

string make_item_uri(const string &item_type_name, const string &id)
{
    return std::format("spotify:{}:{}", item_type_name, id);
}

void from_json(const json &j, simplified_artist_t &a)
{
    j.at("id").get_to(a.id);

    a.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const simplified_artist_t &a)
{
    j = json{
        { "id", a.id },
        { "name", utils::utf8_encode(a.name) },
    };
}

void from_json(const json &j, artist_t &a)
{
    from_json(j, dynamic_cast<simplified_artist_t&>(a));

    j.at("popularity").get_to(a.popularity);
    j.at("genres").get_to(a.genres);
    j.at("images").get_to(a.images);
    j.at("followers").at("total").get_to(a.followers_total);
}

void to_json(json &j, const artist_t &a)
{
    to_json(j, dynamic_cast<const simplified_artist_t&>(a));

    j.update({
        { "popularity", a.popularity },
        { "genres", a.genres },
        { "images", a.images },
        { "followers", { "total", a.followers_total } },
    });
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

void from_json(const json &j, simplified_album_t &a)
{
    j.at("id").get_to(a.id);
    j.at("total_tracks").get_to(a.total_tracks);
    j.at("album_type").get_to(a.album_type);
    j.at("release_date").get_to(a.release_date);
    j.at("release_date_precision").get_to(a.release_date_precision);
    j.at("href").get_to(a.href);
    j.at("images").get_to(a.images);
    j.at("artists").get_to(a.artists);

    a.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const simplified_album_t &a)
{
    j = json{
        { "id", a.id },
        { "total_tracks", a.total_tracks },
        { "album_type", a.album_type },
        { "release_date", a.release_date },
        { "release_date_precision", a.release_date_precision },
        { "href", a.href },
        { "images", a.images },
        { "artists", a.artists },
        { "name", utils::utf8_encode(a.name) },
    };
}

void from_json(const json &j, album_t &a)
{
    from_json(j, dynamic_cast<simplified_album_t&>(a));
}

void to_json(json &j, const album_t &a)
{
    to_json(j, dynamic_cast<const simplified_album_t&>(a));
    //j.update
}

void from_json(const json &j, saved_album_t &a)
{
    from_json(j.at("album"), dynamic_cast<album_t&>(a));
    j.at("added_at").get_to(a.added_at);
}

void to_json(json &j, const saved_album_t &a)
{
    j = json{
        { "added_at", a.added_at },
        { "album", dynamic_cast<const album_t&>(a) },
    };
}

const string& simplified_track_t::get_fields_filter()
{
    static string fields = "id,name,duration_ms,disc_number,track_number,explicit,artists";
    return fields;
}

bool operator==(const simplified_track_t &lhs, const simplified_track_t &rhs)
{
    return lhs.id == rhs.id;
}

void from_json(const json &j, simplified_track_t &t)
{
    j.at("id").get_to(t.id);
    j.at("track_number").get_to(t.track_number);
    j.at("disc_number").get_to(t.disc_number);
    j.at("duration_ms").get_to(t.duration_ms);
    j.at("explicit").get_to(t.is_explicit);
    j.at("artists").get_to(t.artists);

    t.duration = t.duration_ms / 1000;
    t.name = utils::utf8_decode(j.at("name").get<string>());
}

void to_json(json &j, const simplified_track_t &t)
{
    j = json{
        { "id", t.id },
        { "duration_ms", t.duration_ms },
        { "track_number", t.track_number },
        { "disc_number", t.disc_number },
        { "explicit", t.is_explicit },
        { "artists", t.artists },
        { "name", utils::utf8_encode(t.name) },
    };
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

void from_json(const json &j, track_t &t)
{
    from_json(j, dynamic_cast<simplified_track_t&>(t));
    j.at("album").get_to(t.album);
    j.at("artists").get_to(t.artists);
}

void to_json(json &j, const track_t &t)
{
    to_json(j, dynamic_cast<const simplified_track_t&>(t));

    j.update({
        {"album", t.album},
        {"artists", t.artists},
    });
}

const string& saved_track_t::get_fields_filter()
{
    static string fields = std::format("added_at,track({})", track_t::get_fields_filter());
    return fields;
}

void from_json(const json &j, saved_track_t &t)
{
    from_json(j.at("track"), dynamic_cast<track_t&>(t));
    j.at("added_at").get_to(t.added_at);
}

void to_json(json &j, const saved_track_t &t)
{
    j = json{
        { "added_at", t.added_at },
        { "track", dynamic_cast<const track_t&>(t) },
    };
}

void from_json(const json &j, simplified_playlist_t &p)
{
    j.at("id").get_to(p.id);
    j.at("snapshot_id").get_to(p.snapshot_id);
    j.at("href").get_to(p.href);
    j.at("collaborative").get_to(p.collaborative);
    j.at("public").get_to(p.is_public);
    j.at("tracks").at("total").get_to(p.tracks_total);
    
    p.name = utils::utf8_decode(j.at("name").get<string>());
    p.user_display_name = utils::utf8_decode(j.at("owner").at("display_name").get<string>());
    p.description = utils::utf8_decode(j.at("description").get<string>());
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

void from_json(const json &j, playback_state_t &p)
{
    j.at("device").get_to(p.device);
    j.at("repeat_state").get_to(p.repeat_state);
    j.at("shuffle_state").get_to(p.shuffle_state);
    j.at("is_playing").get_to(p.is_playing);
    j.at("actions").get_to(p.actions);

    p.progress_ms = j.value("progress_ms", 0);
    p.progress = p.progress_ms / 1000;

    if (j.contains("context") && !j.at("context").is_null())
        j.at("context").get_to(p.context);

    if (j.contains("item") && !j.at("item").is_null())
        j.at("item").get_to(p.item);
}

void to_json(json &j, const playback_state_t &p)
{
    j = json{
        { "device", p.device },
        { "repeat_state", p.repeat_state },
        { "shuffle_state", p.shuffle_state },
        { "progress_ms", p.progress_ms },
        { "is_playing", p.is_playing },
        { "actions", p.actions },
        { "item", p.item },
        { "context", p.context },
    };
}

void from_json(const json &j, playing_queue_t &p)
{
    if (j.contains("currently_playing") && !j.at("currently_playing").is_null())
        j.at("currently_playing").get_to(p.currently_playing);

    j.at("queue").get_to(p.queue);
}

void to_json(json &j, const playing_queue_t &p)
{
    j = json{
        { "queue", p.queue },
        { "currently_playing", p.currently_playing },
    };
}

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

void from_json(const json &j, actions_t &p)
{
    if (j.is_null())
        return;
    
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

void to_json(json &j, const actions_t &p)
{
    // TODO: unfinished
    j = json{};
}

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


void from_json(const json &j, device_t &d)
{
    j.at("id").get_to(d.id);
    j.at("is_active").get_to(d.is_active);
    j.at("type").get_to(d.type);
    j.at("supports_volume").get_to(d.supports_volume);

    d.volume_percent = j.value("volume_percent", 100);
    d.name = utils::utf8_decode(j.at("name").get<string>());
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

string device_t::to_str() const
{
    return std::format("device(name={}, id={})", utils::to_string(name), id);
}

bool operator==(const device_t &lhs, const device_t &rhs)
{
    return lhs.id == rhs.id;
}

} // namespace spotify
} // namespace spotifar