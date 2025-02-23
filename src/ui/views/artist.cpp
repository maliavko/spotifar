#include "artist.hpp"
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

const wchar_t* artist_view::get_dir_name() const
{
    static wchar_t dir_name[MAX_PATH];
    wcsncpy_s(dir_name, utils::strip_invalid_filename_chars(artist.name).c_str(), MAX_PATH);
    return dir_name;
}

const wchar_t* artist_view::get_title() const
{
    return artist.name.c_str();
}

intptr_t artist_view::select_item(const string &album_id)
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

intptr_t artist_view::process_input(const ProcessPanelInputInfo *info)
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

} // namespace ui
} // namespace spotifar