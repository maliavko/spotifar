#ifndef CONTROLLER_HPP_5BAFAEB4_0F9B_411A_8C57_D6A34B0412FC
#define CONTROLLER_HPP_5BAFAEB4_0F9B_411A_8C57_D6A34B0412FC
#pragma once

#include "spotify/controller.hpp"
#include "spotify/items.hpp"

#include <vector>
#include <string>

namespace spotifar
{
	using std::string;
	using std::wstring;

	class Browser;
	class ViewTarget
	{
	public:
		struct ViewTargetItem
		{
			string id;
			wstring name;
			wstring description;
		};

		typedef std::vector<ViewTargetItem> ItemsCollection;

	public:
		ViewTarget(wstring target_name, string item_id);
		virtual ~ViewTarget() {}

		wstring get_name() const { return target_name; }

		virtual ItemsCollection get_items(Browser& browser) const = 0;

		virtual bool handle_item_selected(Browser& browser, const ItemFarUserData* data) { return false; }

	protected:
		wstring target_name;
		string item_id;
	};

	class Browser
	{
	public:
		Browser();
		~Browser();

		void gotoRootMenu();
		void gotoArtists();
		void gotoArtist(const std::string& id);
		void gotoAlbum(const std::string& id, const std::string& artist_id);
		void gotoPlaylists();

		inline api::Controller& get_api() { return api; }

		wstring get_target_name() const { return current_target->get_name(); }
		ViewTarget::ItemsCollection get_items();

		bool handle_item_selected(const ItemFarUserData* data);

	private:
		std::unique_ptr<ViewTarget> current_target;
		api::Controller api;
	};

	class RootMenuTarget: public ViewTarget
	{
	public:
		RootMenuTarget();
		virtual ItemsCollection get_items(Browser& browser) const;
		virtual bool handle_item_selected(Browser& browser, const ItemFarUserData* data);
	};

	class ArtistsTarget: public ViewTarget
	{
	public:
		ArtistsTarget();
		virtual ItemsCollection get_items(Browser& browser) const;
		virtual bool handle_item_selected(Browser& browser, const ItemFarUserData* data);
	};

	class ArtistTarget: public ViewTarget
	{
	public:
		ArtistTarget(const std::string& id);
		virtual ItemsCollection get_items(Browser& browser) const;
		virtual bool handle_item_selected(Browser& browser, const ItemFarUserData* data);
	private:
		std::string id;
	};

	class AlbumTarget: public ViewTarget
	{
	public:
		AlbumTarget(const std::string& id, const std::string& artist_id);
		virtual ItemsCollection get_items(Browser& browser) const;
		virtual bool handle_item_selected(Browser& browser, const ItemFarUserData* data);
	private:
		std::string id;
		std::string artist_id;
	};

	class PlaylistsTarget: public ViewTarget
	{
	public:
		PlaylistsTarget();
		virtual ItemsCollection get_items(Browser& browser) const;
		virtual bool handle_item_selected(Browser& browser, const ItemFarUserData* data);
	};
}

#endif //CONTROLLER_HPP_5BAFAEB4_0F9B_411A_8C57_D6A34B0412FC
