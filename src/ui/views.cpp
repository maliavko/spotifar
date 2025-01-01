#include "ui/views.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::far3::get_msg;

        const static uintptr_t TMP_FOLDER_ITEM_ATTRS = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
        const static uintptr_t ARTIST_ITEM_ATTRS = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
        const static uintptr_t PLAYLIST_ITEM_ATTRS = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
        const static uintptr_t ALBUM_ITEM_ATTRS = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
        const static uintptr_t TRACK_ITEM_ATTRS = FILE_ATTRIBUTE_VIRTUAL;
        
        ViewItem::ViewItem(const std::string &id, const wstring &name, const wstring &descr,
                           uintptr_t attrs, size_t duration):
            id(id),
            name(utils::strip_invalid_filename_chars(name)),
            description(descr),
            file_attrs(attrs),
            duration(duration)
        {

        }

        std::shared_ptr<RootView> create_root_view()
        {
            return std::make_shared<RootView>();
        }

        std::shared_ptr<ArtistsView> create_artists_view()
        {
            return std::make_shared<ArtistsView>();
        }

        std::shared_ptr<PlaylistsView> create_playlists_view()
        {
            return std::make_shared<PlaylistsView>();
        }

        std::shared_ptr<ArtistView> create_artist_view(const std::string &artist_id)
        {
            return std::make_shared<ArtistView>(artist_id);
        }

        std::shared_ptr<AlbumView> create_album_view(const std::string &album_id, const std::string &artist_id)
        {
            return std::make_shared<AlbumView>(album_id, artist_id);
        }

        View::View(wstring name):
            name(name)
        {

        }

        View::~View()
        {
            
        }

        RootView::RootView():
            View(get_msg(MPanelRootItemLabel))
        {
        }

        View::Items RootView::get_items(Api &api) const
        {
            Items result =
            {
                {
                    ARTISTS_VIEW_ID,
                    get_msg(MPanelArtistsItemLabel),
                    get_msg(MPanelArtistsItemDescr),
                    TMP_FOLDER_ITEM_ATTRS,
                },
                {
                    PLAYLIST_VIEW_ID,
                    get_msg(MPanelPlaylistsItemLabel),
                    get_msg(MPanelPlaylistsItemDescr),
                    TMP_FOLDER_ITEM_ATTRS,
                }
            };

            return result;
        }

        std::shared_ptr<View> RootView::select_item(Api &api, const ItemFarUserData *data)
        {
            if (data->id == ARTISTS_VIEW_ID)
                return create_artists_view();
            
            if (data->id == PLAYLIST_VIEW_ID)
                return create_playlists_view();

            return NULL;
        }
        
        ArtistsView::ArtistsView():
            View(get_msg(MPanelArtistsItemLabel))
        {
        }

        View::Items ArtistsView::get_items(Api &api) const
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, a]: api.get_artists())
                result.push_back({id, a.name, L"", ARTIST_ITEM_ATTRS});
            return result;
        }

        std::shared_ptr<View> ArtistsView::select_item(Api &api, const ItemFarUserData *data)
        {
            if (data == nullptr)
                return create_root_view();
            return create_artist_view(data->id);
        }
        
        ArtistView::ArtistView(const std::string& artist_id):
            View(get_msg(MPanelArtistItemLabel)),
            artist_id(artist_id)
        {
        }

        View::Items ArtistView::get_items(Api& api) const
        {
            // TODO: split albums and singles into separate directoriess
            Items result;
            for (auto& [id, a]: api.get_albums(artist_id))
            {
                std::wstring album_name = std::format(L"[{}] {}", utils::to_wstring(a.get_release_year()), a.name);
                if (a.is_single())
                    album_name += L" [EP]";
                
                result.push_back({id, album_name, L"", ALBUM_ITEM_ATTRS});
            }
            return result;	
        }

        std::shared_ptr<View> ArtistView::select_item(Api &api, const ItemFarUserData *data)
        {
            if (data == nullptr)
                return create_artists_view();
            return create_album_view(data->id, artist_id);
        }
        
        AlbumView::AlbumView(const std::string &album_id, const std::string &artist_id):
            View(get_msg(MPanelAlbumItemLabel)),
            album_id(album_id),
            artist_id(artist_id)
        {
        }

        View::Items AlbumView::get_items(Api &api) const
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, track]: api.get_tracks(album_id))
            {
                std::wstring track_name = std::format(L"{:02}. {}", track.track_number, track.name);
                result.push_back({id, track_name, L"", TRACK_ITEM_ATTRS, (size_t)track.duration});
            }
            return result;
        }

        std::shared_ptr<View> AlbumView::select_item(Api &api, const ItemFarUserData *data)
        {
            if (data == nullptr)
                return create_artist_view(artist_id);

            api.start_playback(album_id, data->id);
            
            return nullptr;
        }
        
        PlaylistsView::PlaylistsView():
            View(get_msg(MPanelPlaylistsItemLabel))
        {
        }

        View::Items PlaylistsView::get_items(Api& api) const
        {
            // TODO: tmp code
            Items result;
            for (auto& [id, a]: api.get_playlists())
                result.push_back({id, a.name, L"", ARTIST_ITEM_ATTRS});
            return result;
        }

        std::shared_ptr<View> PlaylistsView::select_item(Api& api, const ItemFarUserData* data)
        {
            if (data == nullptr)
                return create_root_view();
            return NULL;
        }
    }
}