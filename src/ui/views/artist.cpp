#include "artist.hpp"
#include "album.hpp"
#include "spotify/requests.hpp"
#include "ui/events.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artist_view::artist_view(api_abstract *api, const spotify::artist &artist):
    api_proxy(api),
    artist(artist)
{
    for (const auto &a: api_proxy->get_artist_albums(artist.id))
        items.push_back({
            a.id, a.get_user_name(), L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL
        });
}

auto artist_view::get_dir_name() const -> const wchar_t*
{
    static wchar_t dir_name[MAX_PATH];
    wcsncpy_s(dir_name, utils::strip_invalid_filename_chars(artist.name).c_str(), MAX_PATH);
    return dir_name;
}

auto artist_view::get_title() const -> const wchar_t*
{
    return artist.name.c_str();
}

auto artist_view::select_item(const string &album_id) -> intptr_t
{
    if (album_id.empty())
    {
        events::show_artists_view();
        return TRUE;
    }
    
    const spotify::album &album = api_proxy->get_album(album_id);
    if (album.is_valid())
    { 
        events::show_album_view(artist, album);
        return TRUE;
    }

    return FALSE;
}

auto artist_view::get_find_processor(const string &album_id) -> std::shared_ptr<view::find_processor>
{
    if (!album_id.empty())
        return std::make_shared<album_view::find_processor>(api_proxy, artist.id, album_id);
    
    return nullptr;
}

auto artist_view::process_input(const ProcessPanelInputInfo *info) -> intptr_t
{
    auto& key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        int key = utils::far3::keys::make_combined(key_event);
        switch (key)
        {
            case VK_F4:
            {
                // size_t size = config::ps_info.PanelControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, 0);
                // PluginPanelItem *PPI=(PluginPanelItem*)malloc(size);
                // if (PPI)
                // {
                //     FarGetPluginPanelItem FGPPI={sizeof(FarGetPluginPanelItem), size, PPI};
                //     config::ps_info.PanelControl(PANEL_ACTIVE,FCTL_GETCURRENTPANELITEM, 0, &FGPPI);
                    
                //     const far_user_data* data = nullptr;
                //     if (PPI->UserData.Data != nullptr)
                //     {
                //         data = static_cast<const far_user_data*>(PPI->UserData.Data);
                //         spdlog::debug("current item {}", album::make_uri(data->id));
                //         api->start_playback(album::make_uri(data->id));
                //     }
                    
                //     free(PPI);
                // }

                return TRUE;
            }
        }
    }
    return FALSE;
}

auto artist_view::find_processor::get_items() const -> const items_t*
{
    size_t total_albums = 0;
    string request_url = httplib::append_query_params(
        std::format("/v1/artists/{}/albums", artist_id), {
            { "limit", "1" },
            { "include_groups", "album" }
        });

    auto r = api_proxy->get(request_url, utils::http::session);
    if (utils::http::is_success(r->status))
    {
        json data = json::parse(r->body);
        data["total"].get_to(total_albums);
    }

    static items_t items;
    items.assign({
        // it's a pure fake item, which holds the size of the total amount of followed artists,
        // for the sake of showing it in the item's size column on the panel
        { "", std::format(L"artist albums {}", utils::to_wstring(artist_id)),
            L"", FILE_ATTRIBUTE_VIRTUAL, total_albums }
    });

    return &items;
}

} // namespace ui
} // namespace spotifar