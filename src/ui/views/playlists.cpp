#include "playlists.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;
namespace panels = utils::far3::panels;

playlists_base_view::playlists_base_view(api_abstract *api, const string &view_uid,
    return_callback_t callback):
    view(view_uid, callback),
    api_proxy(api)
{
}

const view::sort_modes_t& playlists_base_view::get_sort_modes() const
{
    using namespace utils::keys;
    static sort_modes_t modes = {
        { L"Name", SM_NAME, VK_F3 + mods::ctrl },
    };
    return modes;
}


const view::items_t* playlists_base_view::get_items()
{
    static view::items_t items; items.clear();

    for (const auto &p: get_playlists())
    {
        std::vector<wstring> columns;

        // column C0 - total playlist's tracks count
        auto tracks = api_proxy->get_playlist_tracks(p.id);
        auto tracks_total = tracks->peek_total();

        wstring tracks_label = L"";
        if (tracks_total > 0)
            tracks_label = std::format(L"{: >6}", tracks_total);
        columns.push_back(tracks_label);

        items.push_back({
            p.id, p.name, L"",
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL,
            columns,
            const_cast<simplified_playlist_t*>(&p)
        });
    }

    return &items;
}

intptr_t playlists_base_view::select_item(const data_item_t* data)
{
    const auto &playlist = api_proxy->get_playlist(data->id);
    if (playlist.is_valid())
    {
        events::show_playlist(api_proxy, playlist);
        return TRUE;
    }

    return FALSE;
}

void playlists_base_view::update_panel_info(OpenPanelInfo *info)
{
    static PanelMode modes[10];

    static const wchar_t* titles_3[] = { L"Name", L"Tracks" };
    modes[3].ColumnTypes = L"NON,C0";
    modes[3].ColumnWidths = L"0,6";
    modes[3].ColumnTitles = titles_3;
    modes[3].StatusColumnTypes = NULL;
    modes[3].StatusColumnWidths = NULL;

    info->PanelModesArray = modes;
    info->PanelModesNumber = std::size(modes);
}

intptr_t playlists_base_view::compare_items(const sort_mode_t &sort_mode,
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

intptr_t playlists_base_view::process_key_input(int combined_key)
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
                    // if (start_playback(user_data->id))
                    // {
                    //     events::show_player();
                    //     return TRUE;
                    // }
                    
                    const auto playlist = static_cast<const simplified_playlist_t*>(user_data);
                    api_proxy->start_playback(playlist->get_uri());
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

bool playlists_base_view::request_extra_info(const data_item_t* data)
{
    if (data != nullptr)
        api_proxy->get_playlist_tracks(data->id)->get_total();

    return false;
}

playlists_view::playlists_view(api_abstract *api):
    playlists_base_view(api, "playlists_view", std::bind(events::show_collections, api)),
    api_proxy(api),
    collection(api_proxy->get_saved_playlists())
{
}

const wstring& playlists_view::get_dir_name() const
{
    static wstring dir_name(get_text(MPanelPlaylistsItemLabel));
    return dir_name;
}

config::settings::view_t playlists_view::get_default_settings() const
{
    return { 0, false, 3 };
}

std::generator<const simplified_playlist_t&> playlists_view::get_playlists()
{
    if (collection->fetch())
        for (const auto &p: *collection)
            co_yield p;
}


recent_playlists_view::recent_playlists_view(api_abstract *api):
    playlists_base_view(api, "recent_playlists_view", std::bind(events::show_recents, api))
{
    rebuild_items();

    utils::events::start_listening<play_history_observer>(this);
}

recent_playlists_view::~recent_playlists_view()
{
    items.clear();

    utils::events::stop_listening<play_history_observer>(this);
}

const wstring& recent_playlists_view::get_dir_name() const
{
    static wstring title(utils::far3::get_text(MPanelPlaylistsItemLabel));
    return title;
}

config::settings::view_t recent_playlists_view::get_default_settings() const
{
    return { 0, false, 3 };
}

const view::sort_modes_t& recent_playlists_view::get_sort_modes() const
{
    using namespace utils::keys;

    static sort_modes_t modes;
    if (!modes.size())
    {
        modes = playlists_base_view::get_sort_modes();
        modes.push_back({ L"Played", SM_MTIME, VK_F6 + mods::ctrl });
    }
    return modes;
}

intptr_t recent_playlists_view::compare_items(const sort_mode_t &sort_mode,
    const data_item_t *data1, const data_item_t *data2)
{
    if (sort_mode.far_sort_mode == SM_MTIME)
    {
        const auto
            &item1 = static_cast<const history_playlist_t*>(data1),
            &item2 = static_cast<const history_playlist_t*>(data2);

        return item1->played_at.compare(item2->played_at);
    }
    return playlists_base_view::compare_items(sort_mode, data1, data2);
}

void recent_playlists_view::rebuild_items()
{
    items.clear();

    std::unordered_map<string, history_item_t> unique_recent_playlists;
    
    // collecting all the tracks' albums listened to
    for (const auto &item: api_proxy->get_play_history())
        if (item.context.is_playlist())
            unique_recent_playlists[item.context.get_item_id()] = item;

    for (const auto &[item_id, item]: unique_recent_playlists)
    {
        auto playlist = api_proxy->get_playlist(item_id);
        if (!playlist.is_valid())
        {
            playlist.id = item_id;
            playlist.name = std::format(L"Forbidden_{}", utils::to_wstring(item_id));
        }

        items.push_back(history_playlist_t(item.played_at, playlist));
    }
}

std::generator<const simplified_playlist_t&> recent_playlists_view::get_playlists()
{
    for (const auto &i: items)
        co_yield i;
}

void recent_playlists_view::on_items_changed()
{
    rebuild_items();
    
    panels::update(PANEL_ACTIVE);
    panels::redraw(PANEL_ACTIVE);
}

} // namespace ui
} // namespace spotifar