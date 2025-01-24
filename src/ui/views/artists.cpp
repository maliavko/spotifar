#include "artists.hpp"
#include "root.hpp"
#include "artist.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;

        ArtistsView::ArtistsView(spotify::Api *api):
            View(get_msg(MPanelArtistsItemLabel)),
            api(api)
        {
            paginator = api->get_library().get_followed_artist(25);
            paginator_it = paginator.begin();

            next_page();
        }

        void ArtistsView::next_page()
        {
            paginator_it++;
            for (const auto &a: *paginator_it)
                artists.push_back(a);
        }

        View::Items ArtistsView::get_items()
        {
            Items result;
            for (const auto &a: artists)
                result.push_back({a.id, a.name, L"", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL});
            return result;
        }

        std::shared_ptr<View> ArtistsView::select_item(const ItemFarUserData *data)
        {
            if (data == nullptr)
                return RootView::create_view(api);
            return ArtistView::create_view(api, data->id);
        }
        
        std::shared_ptr<ArtistsView> ArtistsView::create_view(spotify::Api *api)
        {
            return std::make_shared<ArtistsView>(api);
        }

        void ArtistsView::on_panel_updated(OpenPanelInfo *info)
        {
            PanelInfo pinfo;
            config::PsInfo.PanelControl(PANEL_ACTIVE,FCTL_GETPANELINFO, 0, &pinfo);
            if (artists.size() > 0 && artists.size() - pinfo.CurrentItem < 5)
            {
                next_page();

                spdlog::debug("ArtistsView::on_panel_updated {}/{}", pinfo.CurrentItem, pinfo.ItemsNumber);
			    config::PsInfo.PanelControl(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
            }
        }
    }
}