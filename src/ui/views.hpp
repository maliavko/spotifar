#ifndef VIEWS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#define VIEWS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F
#pragma once

#include "stdafx.h"
#include "spotify/api.hpp"

namespace spotifar
{
    namespace ui
    {
        using std::string;
        using std::wstring;
        using spotify::Api;

        const static string ROOT_VIEW_ID = "/";
        const static string ARTISTS_VIEW_ID = "artists";
        const static string ARTIST_VIEW_ID = "artist";
        const static string ALBUM_VIEW_ID = "album";
        const static string PLAYLISTS_VIEW_ID = "playlists";
        const static string PLAYLIST_VIEW_ID = "playlist";

        struct ViewItem
        {
            string id;
            wstring name;
            wstring description;
            uintptr_t file_attrs;
            size_t duration;

            ViewItem(const string &id, const wstring &name, const wstring &descr,
                uintptr_t attrs = 0, size_t duration = 0);
        };

		struct ItemFarUserData
		{
			string id;
		};

        class View
        {
        public:
            typedef std::vector<ViewItem> Items;

        public:
            View(wstring name);
            virtual ~View();
            virtual string get_id() const = 0;
            inline wstring get_name() const { return name; }

            virtual Items get_items(Api &api) const = 0;
            virtual std::shared_ptr<View> select_item(Api &api, const ItemFarUserData *data) = 0;

        protected:
            wstring name;
        };

        class RootView: public View
        {
        public:
            RootView();

            virtual string get_id() const { return ROOT_VIEW_ID; }
            virtual Items get_items(Api &api) const;
            virtual std::shared_ptr<View> select_item(Api &api, const ItemFarUserData *data);
        };

        class ArtistsView: public View
        {
        public:
            ArtistsView();

            virtual string get_id() const { return ARTISTS_VIEW_ID; }
            virtual Items get_items(Api &api) const;
            virtual std::shared_ptr<View> select_item(Api &api, const ItemFarUserData *data);
        };

        class ArtistView: public View
        {
        public:
            ArtistView(const string &artist_id);

            virtual string get_id() const { return ARTIST_VIEW_ID; }
            virtual Items get_items(Api &api) const;
            virtual std::shared_ptr<View> select_item(Api &api, const ItemFarUserData *data);
        private:
            string artist_id;
        };

        class AlbumView: public View
        {
        public:
            AlbumView(const string &album_id, const string &artist_id);

            virtual string get_id() const { return ALBUM_VIEW_ID; }
            virtual Items get_items(Api &api) const;
            virtual std::shared_ptr<View> select_item(Api &api, const ItemFarUserData *data);
        private:
            string album_id;
            string artist_id;
        };

        class PlaylistsView: public View
        {
        public:
            PlaylistsView();

            virtual string get_id() const { return PLAYLISTS_VIEW_ID; }
            virtual Items get_items(Api &api) const;
            virtual std::shared_ptr<View> select_item(Api &api, const ItemFarUserData *data);
        };

        std::shared_ptr<RootView> create_root_view();
    }
}

#endif //VIEWS_HPP_5219FA27_4DE8_44C0_BF79_6A1310E4805F