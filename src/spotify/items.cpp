#include "items.hpp"
#include "utils.hpp"
#include "lng.hpp"

namespace spotifar { namespace spotify {

using namespace utils::json;
using utils::far3::get_text;

string make_item_uri(const string &item_type_name, const string &id)
{
    return utils::format("spotify:{}:{}", item_type_name, id);
}

bool operator==(const data_item_t &lhs, const data_item_t &rhs)
{
    return lhs.id == rhs.id;
}

void from_json(const Value &j, image_t &i)
{
    i.url = j["url"].GetString();
    i.width = j["width"].GetUint();
    i.height = j["height"].GetUint();
}

void to_json(Value &result, const image_t &i, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    result.AddMember("url", Value(i.url, allocator), allocator);
    result.AddMember("width", Value(i.width), allocator);
    result.AddMember("height", Value(i.height), allocator);
}

void from_json(const json::Value &j, external_urls_t &e)
{
    e.spotify = j["spotify"].GetString();
}

void to_json(json::Value &result, const external_urls_t &e, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    result.AddMember("spotify", Value(e.spotify, allocator), allocator);
}

void from_json(const json::Value &j, copyrights_t &c)
{
    c.type = j["type"].GetString();
    c.text = utils::utf8_decode(j["text"].GetString());
}

void to_json(json::Value &result, const copyrights_t &c, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    result.AddMember("type", Value(c.type, allocator), allocator);
    result.AddMember("text", Value(utils::utf8_encode(c.text), allocator), allocator);
}

void from_json(const Value &j, simplified_artist_t &a)
{
    a.id = j["id"].GetString();
    a.name = utils::utf8_decode(j["name"].GetString());
    
    from_json(j["external_urls"], a.urls);
}

void to_json(Value &result, const simplified_artist_t &a, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value urls;
    to_json(urls, a.urls, allocator);

    result.AddMember("id", Value(a.id, allocator), allocator);
    result.AddMember("name", Value(utils::utf8_encode(a.name), allocator), allocator);
    result.AddMember("external_urls", urls, allocator);
}

const image_t artist_t::get_image() const noexcept
{
    return images.size() > 1 ? images[1] : image_t{};
}

void from_json(const Value &j, artist_t &a)
{
    from_json(j, dynamic_cast<simplified_artist_t&>(a));

    a.popularity = j["popularity"].GetUint();
    a.followers_total = j["followers"]["total"].GetUint();
    
    from_json(j["images"], a.images);
    from_json(j["genres"], a.genres);
}

void to_json(Value &result, const artist_t &a, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    to_json(result, dynamic_cast<const simplified_artist_t&>(a), allocator);

    Value genres;
    to_json(genres, a.genres, allocator);

    Value images;
    to_json(images, a.images, allocator);
    
    auto followers = Value(json::kObjectType);
    followers.AddMember("total", Value(a.followers_total), allocator);

    result.AddMember("id", Value(a.id, allocator), allocator);
    result.AddMember("popularity", Value(a.popularity), allocator);
    result.AddMember("followers", followers, allocator);
    result.AddMember("images", images, allocator);
    result.AddMember("genres", genres, allocator);
}


wstring artist_t::get_main_genre() const
{
    if (genres.size() > 0)
        return utils::to_wstring(genres[0]);
    return get_text(MGenreUnknown);
}

string simplified_album_t::get_release_year() const
{
    static auto r = std::regex("[\\d]{4}");	
    std::smatch match;
    if (std::regex_search(release_date, match, r))
        return match[0];
    return "----";
}

utils::clock_t::time_point simplified_album_t::get_release_date() const
{
    // MSVC std::chrono::parse implementation
    // std::istringstream in{ release_date };
    // utils::clock_t::time_point tp;
    // in >> std::chrono::parse("%F", tp);

    return std::chrono::system_clock::from_time_t(utils::parse_time(release_date, "%Y-%m-%d"));
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

simplified_artist_t simplified_album_t::get_artist() const noexcept
{
    return artists.size() > 0 ? artists[0] : simplified_artist_t{};
}

const image_t simplified_album_t::get_image() const noexcept
{
    return images.size() > 1 ? images[1] : image_t{};
}

const image_t simplified_album_t::get_image_big() const noexcept
{
    return images.size() > 1 ? images[2] : image_t{};
}

wstring simplified_album_t::get_artists_full_name() const
{
    std::vector<wstring> artists_names;
    std::transform(artists.cbegin(), artists.cend(), back_inserter(artists_names),
        [](const auto &a) { return a.name; });
    return utils::string_join(artists_names, L", ");
}

void from_json(const Value &j, simplified_album_t &a)
{
    a.id = j["id"].GetString();
    a.name = utils::utf8_decode(j["name"].GetString());
    a.total_tracks = j["total_tracks"].GetUint();
    a.album_type = j["album_type"].GetString();
    a.release_date = j["release_date"].GetString();
    
    if (j.HasMember("release_date_precision"))
    {
        string precision = j["release_date_precision"].GetString(); // year, month, day
        if (precision == "year")
            a.release_date = utils::format("{}-01-01", a.release_date);
        else if (precision == "month")
            a.release_date = utils::format("{}-01", a.release_date);
    }

    a.href = j["href"].GetString();

    from_json(j["images"], a.images);
    from_json(j["artists"], a.artists);
    from_json(j["external_urls"], a.urls);
}

void to_json(Value &result, const simplified_album_t &a, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value images;
    to_json(images, a.images, allocator);

    Value artists;
    to_json(artists, a.artists, allocator);

    Value urls;
    to_json(urls, a.urls, allocator);

    result.AddMember("id", Value(a.id, allocator), allocator);
    result.AddMember("name", Value(utils::utf8_encode(a.name), allocator), allocator);
    result.AddMember("total_tracks", Value(a.total_tracks), allocator);
    result.AddMember("album_type", Value(a.album_type, allocator), allocator);
    result.AddMember("release_date", Value(a.release_date, allocator), allocator);
    result.AddMember("href", Value(a.href, allocator), allocator);
    result.AddMember("images", images, allocator);
    result.AddMember("artists", artists, allocator);
    result.AddMember("external_urls", urls, allocator);
}

copyrights_t album_t::get_main_copyright() const
{
    if (copyrights.size() > 0)
        return copyrights[0];
    return { "C", get_text(MCopyrightUnknown) };
}

void from_json(const Value &j, album_t &a)
{
    from_json(j, dynamic_cast<simplified_album_t&>(a));
    from_json(j["copyrights"], a.copyrights);
    // from_json(j["items"], a.tracks);
    
    a.popularity = j["popularity"].GetUint();
    a.recording_label = utils::utf8_decode(j["label"].GetString());
}

void to_json(Value &result, const album_t &a, json::Allocator &allocator)
{
    result = Value(json::kObjectType);
    to_json(result, dynamic_cast<const simplified_album_t&>(a), allocator);
    
    Value copyrights;
    to_json(copyrights, a.copyrights, allocator);
    
    // Value tracks;
    // to_json(tracks, a.tracks, allocator);
    
    // result.AddMember("items", tracks, allocator);
    result.AddMember("copyrights", copyrights, allocator);
    result.AddMember("popularity", Value(a.popularity), allocator);
    result.AddMember("label", Value(utils::utf8_encode(a.recording_label), allocator), allocator);
}

void to_json(Value &result, saved_album_t &a, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value album;
    to_json(album, dynamic_cast<const album_t&>(a), allocator);

    result.AddMember("added_at", Value(a.added_at, allocator), allocator);
    result.AddMember("album", album, allocator);
}

void from_json(const Value &j, saved_album_t &a)
{
    from_json(j["album"], dynamic_cast<album_t&>(a));
    a.added_at = j["added_at"].GetString();
}

const string& simplified_track_t::get_fields_filter()
{
    static string fields = "id,name,duration_ms,disc_number,track_number,explicit,artists,external_urls";
    return fields;
}

simplified_artist_t simplified_track_t::get_artist() const noexcept
{
    return artists.size() > 0 ? artists[0] : simplified_artist_t{};
}

wstring simplified_track_t::get_artists_full_name() const
{
    std::vector<wstring> artists_names;
    std::transform(artists.cbegin(), artists.cend(), back_inserter(artists_names),
        [](const auto &a) { return a.name; });
    return utils::string_join(artists_names, L", ");
}

void from_json(const Value &j, simplified_track_t &t)
{
    t.id = j["id"].GetString();
    t.track_number = j["track_number"].GetUint();
    t.disc_number = j["disc_number"].GetUint();
    t.duration_ms = j["duration_ms"].GetInt();
    t.is_explicit = j["explicit"].GetBool();
    
    t.duration = t.duration_ms / 1000;
    t.name = utils::utf8_decode(j["name"].GetString());
    
    from_json(j["artists"], t.artists);
    from_json(j["external_urls"], t.urls);
}

void to_json(Value &result, const simplified_track_t &t, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value artists;
    to_json(artists, t.artists, allocator);

    Value urls;
    to_json(urls, t.urls, allocator);

    result.AddMember("id", Value(t.id, allocator), allocator);
    result.AddMember("name", Value(utils::utf8_encode(t.name), allocator), allocator);
    result.AddMember("duration_ms", Value(t.duration_ms), allocator);
    result.AddMember("track_number", Value(t.track_number), allocator);
    result.AddMember("disc_number", Value(t.disc_number), allocator);
    result.AddMember("explicit", Value(t.is_explicit), allocator);
    result.AddMember("artists", artists, allocator);
    result.AddMember("external_urls", urls, allocator);
}

const string& track_t::get_fields_filter()
{
    static string fields = utils::format("{},album,popularity", simplified_track_t::get_fields_filter());
    return fields;
}

void from_json(const Value &j, track_t &t)
{
    from_json(j, dynamic_cast<simplified_track_t&>(t));
    from_json(j["album"], t.album);
    
    t.popularity = j["popularity"].GetUint();
}

void to_json(Value &result, const track_t &t, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    to_json(result, dynamic_cast<const simplified_track_t&>(t), allocator);

    Value album;
    to_json(album, t.album, allocator);

    result.AddMember("album", album, allocator);
    result.AddMember("popularity", Value(t.popularity), allocator);
}

const string& saved_track_t::get_fields_filter()
{
    static string fields = utils::format("added_at,track({})", track_t::get_fields_filter());
    return fields;
}

void to_json(Value &result, const saved_track_t &t, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value track;
    to_json(track, dynamic_cast<const track_t&>(t), allocator);

    result.AddMember("added_at", Value(t.added_at, allocator), allocator);
    result.AddMember("track", track, allocator);
}
    
void from_json(const Value &j, saved_track_t &t)
{
    from_json(j["track"], dynamic_cast<track_t&>(t));
    t.added_at = j["added_at"].GetString();
}

void from_json(const Value &j, simplified_playlist_t &p)
{
    p.id = j["id"].GetString();
    p.snapshot_id = j["snapshot_id"].GetString();
    p.href = j["href"].GetString();
    p.is_collaborative = j["collaborative"].GetBool();
    p.is_public = j["public"].GetBool();
    p.tracks_total = j["tracks"]["total"].GetUint();
    
    p.name = utils::utf8_decode(j["name"].GetString());
    p.user_display_name = utils::utf8_decode(j["owner"]["display_name"].GetString());
    p.description = utils::utf8_decode(j["description"].GetString());
    
    from_json(j["external_urls"], p.urls);
}

void to_json(Value &result, const simplified_playlist_t &p, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value tracks;
    tracks.AddMember("total", Value(p.tracks_total), allocator);

    Value owner;
    owner.AddMember("name", Value(utils::utf8_encode(p.user_display_name), allocator), allocator);

    Value urls;
    to_json(urls, p.urls, allocator);

    result.AddMember("id", Value(p.id, allocator), allocator);
    result.AddMember("snapshot_id", Value(p.snapshot_id, allocator), allocator);
    result.AddMember("name", Value(utils::utf8_encode(p.name), allocator), allocator);
    result.AddMember("description", Value(utils::utf8_encode(p.description), allocator), allocator);
    result.AddMember("href", Value(p.href, allocator), allocator);
    result.AddMember("collaborative", Value(p.is_collaborative), allocator);
    result.AddMember("public", Value(p.is_public), allocator);
    result.AddMember("tracks", tracks, allocator);
    result.AddMember("actions", owner, allocator);
    result.AddMember("external_urls", urls, allocator);
}

void to_json(Value &result, const playlist_t &a, json::Allocator &allocator)
{
    to_json(result, dynamic_cast<const simplified_playlist_t&>(a), allocator);
}

void from_json(const Value &j, playlist_t &p)
{
    from_json(j, dynamic_cast<simplified_playlist_t&>(p));
}

const string& simplified_playlist_t::get_fields_filter()
{
    static string fields = utils::format("id,href,name,collaborative,public,description,"
        "tracks(total),owner(display_name),snapshot_id,external_urls");
    return fields;
}

simplified_playlist_t simplified_playlist_t::make_hidden(const item_id_t &id, const wstring &name)
{
    return {
        { id },
        name,
        true,   // is hidden
        "",     // href
        "",     // snapshot id
        get_text(MSpotifyOwner), // user name
        false,      // is collaborative
        false,      // is public
        get_text(MSpotifyDescr),
        0,      // total tracks
        { "", } // urls
    };
}

const string& playlist_t::get_fields_filter()
{
    static string fields = utils::format("{}", simplified_playlist_t::get_fields_filter());
    return fields;
}

void from_json(const Value &j, playback_state_t &p)
{
    from_json(j["device"], p.device);
    from_json(j["actions"], p.actions);

    if (j.HasMember("context") && !j["context"].IsNull())
        from_json(j["context"], p.context);

    if (j.HasMember("item") && !j["item"].IsNull())
        from_json(j["item"], p.item);

    p.progress_ms = j.HasMember("progress_ms") ? j["progress_ms"].GetInt() : 0;
    p.progress = p.progress_ms / 1000;

    p.repeat_state = j["repeat_state"].GetString();
    p.shuffle_state = j["shuffle_state"].GetBool();
    p.is_playing = j["is_playing"].GetBool();
}

void to_json(Value &result, const playback_state_t &p, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value device;
    to_json(device, p.device, allocator);

    Value actions;
    to_json(actions, p.actions, allocator);

    Value item;
    to_json(item, p.item, allocator);

    Value context;
    to_json(context, p.context, allocator);

    result.AddMember("repeat_state", Value(p.repeat_state, allocator), allocator);
    result.AddMember("shuffle_state", Value(p.shuffle_state), allocator);
    result.AddMember("progress_ms", Value(p.progress_ms), allocator);
    result.AddMember("is_playing", Value(p.is_playing), allocator);
    result.AddMember("device", device, allocator);
    result.AddMember("actions", actions, allocator);
    result.AddMember("item", item, allocator);
    result.AddMember("context", context, allocator);
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

void from_json(const Value &j, actions_t &a)
{
    if (j.HasMember("disallows"))
    {
        const auto &actions = j["disallows"];

        // data contains disallowing flags, so we need to invert them in order to
        // comply the other code usage
        auto is_allowed = [&](const string &fieldname) -> bool
        {
            return actions.HasMember(fieldname) ? !actions[fieldname].GetBool() : true;
        };
    
        a.interrupting_playback = is_allowed("interrupting_playback");
        a.pausing = is_allowed("pausing");
        a.resuming = is_allowed("resuming");
        a.seeking = is_allowed("seeking");
        a.skipping_next = is_allowed("skipping_next");
        a.skipping_prev = is_allowed("skipping_prev");
        a.toggling_repeat_context = false;
        a.toggling_repeat_track = false;
        a.toggling_repeat_context = is_allowed("toggling_repeat_context");
        a.toggling_repeat_track = is_allowed("toggling_repeat_track");
        a.toggling_shuffle = is_allowed("toggling_shuffle");
        a.trasferring_playback = is_allowed("trasferring_playback");
    }
}

void to_json(Value &result, const actions_t &a, json::Allocator &al)
{
    result = Value(json::kObjectType);
    result.AddMember("disallows", Value(json::kObjectType), al);

    auto &disallows = result["disallows"];
    
    // helper to add only the positive flags into the object
    auto write_action = [&](const string &fieldname, bool value)
    {
        if (!value)
            disallows.AddMember(Value(fieldname, al), Value(true), al);
    };

    write_action("interrupting_playback", a.interrupting_playback);
    write_action("pausing", a.pausing);
    write_action("resuming", a.resuming);
    write_action("seeking", a.seeking);
    write_action("skipping_next", a.skipping_next);
    write_action("skipping_prev", a.skipping_prev);
    write_action("toggling_repeat_context", a.toggling_repeat_context);
    write_action("toggling_repeat_track", a.toggling_repeat_track);
    write_action("toggling_shuffle", a.toggling_shuffle);
    write_action("trasferring_playback", a.trasferring_playback);
}

void from_json(const Value &j, context_t &c)
{
    c.type = j["type"].GetString();
    c.uri = j["uri"].GetString();
    c.href = j["href"].GetString();
}

void to_json(Value &result, const context_t &c, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    result.AddMember("type", Value(c.type, allocator), allocator);
    result.AddMember("uri", Value(c.uri, allocator), allocator);
    result.AddMember("href", Value(c.href, allocator), allocator);
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

void from_json(const Value &j, device_t &d)
{
    d.id = j["id"].GetString();
    d.is_active = j["is_active"].GetBool();
    d.type = j["type"].GetString();
    d.supports_volume = j["supports_volume"].GetBool();
    d.is_private_session = j["is_private_session"].GetBool();
    d.is_restricted = j["is_restricted"].GetBool();
    d.name = utils::utf8_decode(j["name"].GetString());

    d.volume_percent = j.HasMember("volume_percent") ? j["volume_percent"].GetInt() : 0;
}

void to_json(Value &result, const device_t &d, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    result.AddMember("id", Value(d.id, allocator), allocator);
    result.AddMember("name", Value(utils::utf8_encode(d.name), allocator), allocator);
    result.AddMember("is_active", Value(d.is_active), allocator);
    result.AddMember("type", Value(d.type, allocator), allocator);
    result.AddMember("volume_percent", Value(d.volume_percent), allocator);
    result.AddMember("supports_volume", Value(d.supports_volume), allocator);
    result.AddMember("is_private_session", Value(d.is_private_session), allocator);
    result.AddMember("is_restricted", Value(d.is_restricted), allocator);
}

string device_t::to_str() const
{
    return utils::format("device(name={}, id={})", utils::to_string(name), id);
}

void to_json(Value &result, const history_item_t &i, json::Allocator &allocator)
{
    result = Value(json::kObjectType);
    
    Value track;
    to_json(track, dynamic_cast<const track_t&>(i), allocator);

    Value context;
    to_json(context, i.context, allocator);

    result.AddMember("played_at", Value(i.played_at, allocator), allocator);
    result.AddMember("context", context, allocator);
    result.AddMember("track", track, allocator);
}
    
void from_json(const Value &j, history_item_t &p)
{
    from_json(j["track"], dynamic_cast<track_t&>(p));

    p.played_at = j["played_at"].GetString();

    if (!j["context"].IsNull())
        from_json(j["context"], p.context);
}

void to_json(Value &result, const playing_queue_t &p, json::Allocator &allocator)
{
    result = Value(json::kObjectType);

    Value currently_playing;
    to_json(currently_playing, p.currently_playing, allocator);

    Value queue;
    to_json(queue, p.queue, allocator);

    result.AddMember("currently_playing", currently_playing, allocator);
    result.AddMember("queue", queue, allocator);
}

void from_json(const Value &j, playing_queue_t &p)
{
    if (!j["currently_playing"].IsNull())
        from_json(j["currently_playing"], p.currently_playing);

    from_json(j["queue"], p.queue);
}

void from_json(const Value &j, auth_t &a)
{
    a.access_token = j["access_token"].GetString();
    a.scope = j["scope"].GetString();
    a.expires_in = j["expires_in"].GetInt();
    
    if (j.HasMember("refresh_token") && !j["refresh_token"].IsNull())
        a.refresh_token = j["refresh_token"].GetString();
    else
        a.refresh_token = "";
}

void to_json(Value &result, const auth_t &a, json::Allocator &allocator)
{
    result = Value(kObjectType);

    result.AddMember("access_token", Value(a.access_token, allocator), allocator);
    result.AddMember("scope", Value(a.scope, allocator), allocator);
    result.AddMember("expires_in", Value(a.expires_in), allocator);
    result.AddMember("refresh_token", Value(a.refresh_token, allocator), allocator);
}

} // namespace spotify
} // namespace spotifar