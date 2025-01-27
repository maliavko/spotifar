#include "artist.hpp"
#include "artists.hpp"
#include "album.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;
        
        ArtistView::ArtistView(Api *api, const Artist &artist):
            View(artist.name),
            api(api),
            artist(artist)
        {
        }

        View::Items ArtistView::get_items()
        {
            // TODO: split albums and singles into separate directoriess
            Items result;
            for (auto &[id, a]: api->get_albums(artist.id))
            {
                std::wstring album_name = std::format(L"[{}] {}", utils::to_wstring(a.get_release_year()), a.name);
                if (a.is_single())
                    album_name += L" [EP]";
                
                result.push_back({id, album_name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
            }
            return result;
        }

        std::shared_ptr<View> ArtistView::select_item(const ItemFarUserData *data)
        {
            if (data == nullptr)
                return ArtistsView::create_view(api);
            return AlbumView::create_view(api, data->id, artist.id);
        }
        
        intptr_t ArtistView::process_input(const ProcessPanelInputInfo *info)
        {
            auto& key_event = info->Rec.Event.KeyEvent;
            if (key_event.bKeyDown)
            {
                int key = utils::far3::input_record_to_combined_key(key_event);
                switch (key)
                {
                    case VK_F4:
                    {
                        size_t size = config::PsInfo.PanelControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, 0);
                        PluginPanelItem *PPI=(PluginPanelItem*)malloc(size);
                        if (PPI)
                        {
                            FarGetPluginPanelItem FGPPI={sizeof(FarGetPluginPanelItem), size, PPI};
                            config::PsInfo.PanelControl(PANEL_ACTIVE,FCTL_GETCURRENTPANELITEM, 0, &FGPPI);
                            
                            const ItemFarUserData* data = nullptr;
                            if (PPI->UserData.Data != nullptr)
                            {
                                data = static_cast<const ItemFarUserData*>(PPI->UserData.Data);
                                spdlog::debug("current item {}", Album::make_uri(data->id));
                                api->start_playback(Album::make_uri(data->id));
                            }
                            
                            free(PPI);
                        }

                        return TRUE;
                    }
                }
            }
            return FALSE;
        }

        std::shared_ptr<ArtistView> ArtistView::create_view(Api *api, const Artist &artist)
        {
            return std::make_shared<ArtistView>(api, artist);
        }
    }
}