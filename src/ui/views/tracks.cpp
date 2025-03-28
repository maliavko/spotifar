#include "tracks.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

namespace panels = utils::far3::panels;

tracks_base_view::tracks_base_view(api_abstract *api, const string &view_uid,
                                   return_callback_t callback):
    view(view_uid, callback),
    api_proxy(api)
{
}

const view::sort_modes_t& tracks_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name",          SM_NAME,    VK_F3 + mods::ctrl },
        { L"Track Number",  SM_EXT,     VK_F4 + mods::ctrl },
        { L"Duration",      SM_SIZE,    VK_F5 + mods::ctrl },
    };
    return modes;
}

const view::items_t* tracks_base_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &track: get_tracks())
    {
        std::vector<wstring> columns;
        
        // column C0 - is explicit lyrics
        columns.push_back(track.is_explicit ? L" * " : L"");

        // column C1 - duration
        auto duration = std::chrono::milliseconds(track.duration_ms);
        wstring track_length;
        if (duration < 1h)
            track_length = std::format(L"{:%M:%S}", duration);
        else
            track_length = std::format(L"{:%Hh%M}", duration);
        columns.push_back(track_length.substr(0, 5));
        
        // column C2 - track number
        // eaither just 01,02 or in a multisdics case - 1/02, 1/02 etc.
        wstring track_number = std::format(L"{:02}", track.track_number);
        columns.push_back(std::format(L"{: ^7}", track_number));

        // list of artists is used as a description field
        std::vector<wstring> artists_names;
        std::transform(track.artists.cbegin(), track.artists.cend(), back_inserter(artists_names),
            [](const auto &a) { return a.name; });
        
        const auto &pstate = api_proxy->get_playback_state();

        items.push_back({
            track.id,
            track.name,
            utils::string_join(artists_names, L", "),
            FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<simplified_track_t*>(&track),
            pstate.item.is_valid() && pstate.item.id == track.id
        });
    }

    return &items;
}

void tracks_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    // TODO: if there is no multidics, the first column can be shorter?
    // or make it the same to Spotify - no disc number, just sorting
    static const wchar_t* titles_3[] = { L"##", L"Name", L"[E]", L"Time" };
    modes[3].ColumnTypes = L"C2,NON,C0,C1";
    modes[3].ColumnWidths = L"7,0,3,5";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

intptr_t tracks_base_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    const auto
        &item1 = static_cast<const simplified_track_t*>(data1),
        &item2 = static_cast<const simplified_track_t*>(data2);

    switch (sort_mode.far_sort_mode)
    {
        case SM_NAME:
            return item1->name.compare(item2->name);

        case SM_EXT:
            if (item1->disc_number == item2->disc_number)
            {
                if (item1->track_number == item2->track_number)
                    return 0;
                return item1->track_number < item2->track_number ? -1 : 1;
            }
            return item1->disc_number < item2->disc_number ? -1 : 1;

        case SM_SIZE:
            if (item1->duration_ms == item2->duration_ms)
                return 0;
            return item1->duration_ms < item2->duration_ms ? -1 : 1;
    }
    return -2;
}

intptr_t tracks_base_view::process_key_input(int combined_key)
{
    switch (combined_key)
    {
        case VK_RETURN + utils::keys::mods::shift:
        {
            auto item = panels::get_current_item(PANEL_ACTIVE);
            if (item != nullptr)
            {
                if (auto *user_data = unpack_user_data(item->UserData))
                {
                    utils::log::global->info("Starting playback from the tracks view, {}", user_data->id);
                    // if (start_playback(user_data->id))
                    // {
                    //     events::show_player();
                    //     return TRUE;
                    // }
                    start_playback(user_data->id);
                    return TRUE;
                }
            }
            else
                utils::log::global->error("There is an error occured while getting a current panel item");

            return TRUE;
        }
    }
    return FALSE;
}

album_tracks_view::album_tracks_view(api_abstract *api, const album_t &album,
                                     return_callback_t callback):
    tracks_base_view(api, "album_tracks_view", callback),
    album(album),
    collection(api_proxy->get_album_tracks(album.id))
{
    utils::events::start_listening<playback_observer>(this);
}

album_tracks_view::~album_tracks_view()
{
    utils::events::stop_listening<playback_observer>(this);
}

const wstring& album_tracks_view::get_dir_name() const
{
    return album.name;
}

config::settings::view_t album_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

bool album_tracks_view::start_playback(const string &track_id)
{
    api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const simplified_track_t&> album_tracks_view::get_tracks()
{
    if (collection->fetch())
        for (const auto &t: *collection)
            co_yield t;
}

void album_tracks_view::on_track_changed(const track_t &track)
{
    if (album.id == track.album.id) // the currently playing track is from this album
    {
        panels::update(PANEL_ACTIVE);
        panels::redraw(PANEL_ACTIVE);

        // experimental code to select the currently playing item on the panel
        // panels::clear_selection(PANEL_ACTIVE);

        // if (auto track_idx = get_item_idx(track.id))
        //     panels::select_item(PANEL_ACTIVE, track_idx);
    }
}

recent_tracks_view::recent_tracks_view(api_abstract *api):
    tracks_base_view(api, "recent_tracks_view", std::bind(events::show_recents, api))
{
    rebuild_items();

    utils::events::start_listening<play_history_observer>(this);
}

recent_tracks_view::~recent_tracks_view()
{
    items.clear();

    utils::events::stop_listening<play_history_observer>(this);
}

const wstring& recent_tracks_view::get_dir_name() const
{
    static wstring title(utils::far3::get_text(MPanelTracksItemLabel));
    return title;
}

config::settings::view_t recent_tracks_view::get_default_settings() const
{
    return { 0, false, 3 };
}

const view::sort_modes_t& recent_tracks_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = tracks_base_view::get_sort_modes();
        modes.push_back({ L"Played", SM_MTIME, VK_F6 + mods::ctrl });
    }
    return modes;
}

intptr_t recent_tracks_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_track_t*>(data1),
            &item2 = static_cast<const history_track_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return tracks_base_view::compare_items(sort_mode, data1, data2);
}

void recent_tracks_view::rebuild_items()
{
    items.clear();

    for (const auto &item: api_proxy->get_play_history())
        items.push_back(
            history_track_t(item.played_at, item.track)
        );
}

bool recent_tracks_view::start_playback(const string &track_id)
{
    //api_proxy->start_playback(album.get_uri(), track_t::make_uri(track_id));
    return true;
}

std::generator<const simplified_track_t&> recent_tracks_view::get_tracks()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_tracks_view::on_items_changed()
{
    rebuild_items();
    
    panels::update(PANEL_ACTIVE);
    panels::redraw(PANEL_ACTIVE);
}

} // namespace ui
} // namespace spotifar