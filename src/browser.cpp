#include "stdafx.h"

#include "browser.hpp"
#include "config.hpp"
#include "lng.hpp"


namespace spotifar
{
	using config::get_msg;

	auto& cfg = config::Opt;

	Browser::Browser():
		api(config::to_str(cfg.SpotifyClientID), config::to_str(cfg.SpotifyClientSecret),
			cfg.LocalhostServicePort, config::to_str(cfg.SpotifyRefreshToken))
	{
		api.authenticate();
		gotoRootMenu();  // initializing root menu by default
	}

	Browser::~Browser()
	{
		config::set_str(cfg.SpotifyRefreshToken, api.get_refresh_token());
	}
 
	void Browser::gotoRootMenu()
	{
		current_target = std::make_unique<RootMenuTarget>();
	}
 
	void Browser::gotoArtists()
	{
		// TODO: process correctly selected item on the panel
		// https://api.farmanager.com/ru/structures/pluginpanelitem.html
		// PPIF_SELECTED
		current_target = std::make_unique<ArtistsTarget>();
	}
 
	void Browser::gotoArtist(const std::string& id)
	{
		current_target = std::make_unique<ArtistTarget>(id);
	}
 
	void Browser::gotoAlbum(const std::string& id, const std::string& artist_id)
	{
		current_target = std::make_unique<AlbumTarget>(id, artist_id);
	}
 
	void Browser::gotoPlaylists()
	{
		current_target = std::make_unique<PlaylistsTarget>();
	}

	ViewTarget::ItemsCollection Browser::get_items()
	{
		return current_target->get_items(*this);
	}

	bool Browser::handle_item_selected(const ItemFarUserData* data)
	{
		return current_target->handle_item_selected(*this, data);
	}
	
	ViewTarget::ViewTarget(wstring target_name_, std::string item_id_):
		target_name(target_name_), item_id(item_id_)
	{
	}
	
	// empty name is handled to Far, which is used as VirtualPanel "Dir"
	// attribute; when it's empty, Far closes the panel when ".." item is hit
	RootMenuTarget::RootMenuTarget():
		ViewTarget(get_msg(MPanelRootItemLabel), "")
	{
	}

	ViewTarget::ItemsCollection RootMenuTarget::get_items(Browser& browser) const
	{
		ItemsCollection result =
		{
			{
				"artists",
				get_msg(MPanelArtistsItemLabel),
				get_msg(MPanelArtistsItemDescr)
			},
			{
				"playlists",
				get_msg(MPanelPlaylistsItemLabel),
				get_msg(MPanelPlaylistsItemDescr),
			}
		};

		return result;
	}

	bool RootMenuTarget::handle_item_selected(Browser& browser, const ItemFarUserData* data)
	{
		if (data->id == "artists")
		{
			browser.gotoArtists();
			return true;
		}
		else if (data->id == "playlists")
		{
			browser.gotoPlaylists();
			return true;
		}
		return false;
	}
	
	ArtistsTarget::ArtistsTarget():
		ViewTarget(get_msg(MPanelArtistsItemLabel), "")
	{
	}

	ViewTarget::ItemsCollection ArtistsTarget::get_items(Browser& browser) const
	{
		// TODO: tmp code
		ItemsCollection result;
		for (auto& [id, a]: browser.get_api().get_artist())
		{
			result.push_back({
				id, utils::to_wstring(a.name), L""
			});
		}
		return result;
	}

	bool ArtistsTarget::handle_item_selected(Browser& browser, const ItemFarUserData* data)
	{
		if (data == nullptr)
		{
			browser.gotoRootMenu();
			return true;
		}

		browser.gotoArtist(data->id);
		return true;
	}
	
	ArtistTarget::ArtistTarget(const std::string& id):
		ViewTarget(get_msg(MPanelArtistsItemLabel), ""),  // TODO: put a normal name to be seen in the panel title
		id(id)
	{
	}

	ViewTarget::ItemsCollection ArtistTarget::get_items(Browser& browser) const
	{
		// TODO: tmp code
		ItemsCollection result;
		for (auto& [id, a]: browser.get_api().get_albums(id))
		{
			result.push_back({
				id, utils::to_wstring(a.name), L""
			});
		}
		return result;	
	}

	bool ArtistTarget::handle_item_selected(Browser& browser, const ItemFarUserData* data)
	{
		if (data == nullptr)
		{
			browser.gotoArtists();
			return true;
		}

		browser.gotoAlbum(data->id, id);
		return false;
	}
	
	AlbumTarget::AlbumTarget(const std::string& id, const std::string& artist_id):
		ViewTarget(get_msg(MPanelArtistsItemLabel), ""),  // TODO: put a normal name to be seen in the panel title
		id(id),
		artist_id(artist_id)
	{
	}

	ViewTarget::ItemsCollection AlbumTarget::get_items(Browser& browser) const
	{
		// TODO: tmp code
		ItemsCollection result;
		for (auto& [id, a]: browser.get_api().get_tracks(id))
		{
			std::string track_user_name = std::format("{:2}. {}", a.track_number, a.name);
			result.push_back({
				id, utils::to_wstring(track_user_name), L""
			});
		}
		return result;	
	}

	bool AlbumTarget::handle_item_selected(Browser& browser, const ItemFarUserData* data)
	{
		if (data == nullptr)
		{
			browser.gotoArtist(artist_id);
			return true;
		}

		browser.get_api().start_playback(id, data->id);
		
		return false;
	}
	
	PlaylistsTarget::PlaylistsTarget():
		ViewTarget(get_msg(MPanelPlaylistsItemLabel), "")
	{
	}

	ViewTarget::ItemsCollection PlaylistsTarget::get_items(Browser& browser) const
	{
		// TODO: tmp code
		ItemsCollection result =
		{
			{
				"playlist1",
				L"Playlist1",
				L"Playlist1"
			},
			{
				"playlist2",
				L"Playlist2",
				L"Playlist2"
			}
		};
		return result;	
	}

	bool PlaylistsTarget::handle_item_selected(Browser& browser, const ItemFarUserData* data)
	{
		if (data == nullptr)
		{
			browser.gotoRootMenu();
			return true;
		}
		return false;
	}
}
