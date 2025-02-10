#include "artist.hpp"
#include "artists.hpp"
#include "album.hpp"

namespace spotifar { namespace ui {

using utils::far3::get_text;

artist_view::artist_view(spotify::api *api, const spotify::artist &artist):
    view(artist.name),
    api(api),
    artist(artist)
{
}

view::view_items_t artist_view::get_items()
{
    // TODO: split albums and singles into separate directoriess
    view_items_t result;
    for (auto &[id, a]: api->get_albums(artist.id))
    {
        result.push_back({id, a.get_user_name(), L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
    }
    return result;
}

std::shared_ptr<view> artist_view::select_item(const string &album_id)
{
    if (album_id.empty())
        return artists_view::build(api);
    
    const spotify::album *album = api->get_library().get_album(album_id);
    if (album != nullptr)
        return album_view::build(api, *album, artist);

    return nullptr;
}

intptr_t artist_view::process_input(const ProcessPanelInputInfo *info)
{
    auto& key_event = info->Rec.Event.KeyEvent;
    if (key_event.bKeyDown)
    {
        int key = utils::far3::input_record_to_combined_key(key_event);
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

std::shared_ptr<artist_view> artist_view::build(
    spotify::api *api, const spotify::artist &artist)
{
    return std::make_shared<artist_view>(api, artist);
}

} // namespace ui
} // namespace spotifar