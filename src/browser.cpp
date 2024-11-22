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
		current_target = std::make_unique<ArtistsTarget>();
	}
 
	void Browser::gotoPlaylists()
	{
		current_target = std::make_unique<PlaylistsTarget>();
	}

	ViewTarget::ItemsCollection Browser::get_items()
	{
		return current_target->get_items(*this);
	}

	bool Browser::handle_item_selected(wstring item_name)
	{
		return current_target->handle_item_selected(*this, item_name);
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
				get_msg(MPanelArtistsItemLabel),
				get_msg(MPanelArtistsItemDescr)
			},
			{
				get_msg(MPanelPlaylistsItemLabel),
				get_msg(MPanelPlaylistsItemDescr),
			}
		};

		return result;
	}

	bool RootMenuTarget::handle_item_selected(Browser& browser, wstring item_name)
	{
		if (item_name == get_msg(MPanelArtistsItemLabel))
		{
			browser.gotoArtists();
			return true;
		}
		else if (item_name == get_msg(MPanelPlaylistsItemLabel))
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
				utils::to_wstring(a.name), L""
			});
		}
		return result;	
	}

	bool ArtistsTarget::handle_item_selected(Browser& browser, wstring item_name)
	{
		if (item_name.empty() || item_name == L"..")
		{
			browser.gotoRootMenu();
			return true;
		}
		return false;
	}
	
	PlaylistsTarget::PlaylistsTarget():
		ViewTarget(get_msg(MPanelPlaylistsItemLabel), "")
	{
	}

	ViewTarget::ItemsCollection PlaylistsTarget::get_items(Browser& browser) const
	{
		ItemsCollection result =
		{
			{
				L"Playlist1",
				L"Playlist1"
			},
			{
				L"Playlist2",
				L"Playlist2"
			}
		};
		return result;	
	}

	bool PlaylistsTarget::handle_item_selected(Browser& browser, wstring item_name)
	{
		if (item_name.empty() || item_name == L"..")
		{
			browser.gotoRootMenu();
			return true;
		}
		return false;
	}
}
