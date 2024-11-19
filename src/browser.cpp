#include "stdafx.h"

#include "browser.hpp"
#include "config.hpp"
#include "lng.hpp"


namespace spotifar
{
	using config::get_msg;

	auto& cfg = config::Opt;

	Browser::Browser()
	{
		gotoRootMenu();

		start_relay();

		api.login();
	}

	Browser::~Browser()
	{
		stop_relay();
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

	bool Browser::start_relay()
	{
		ZeroMemory(&relay_si, sizeof(relay_si));
		relay_si.cb = sizeof(relay_si);
		ZeroMemory(&relay_pi, sizeof(relay_pi));

		std::wstring path = std::format(
			L"{}\\spotify-http-relay.exe --port {} --client-id {} --client-secret {}",
			cfg.PluginStartupFolder,
			cfg.LocalhostServicePort,
			cfg.SpotifyClientID,
			cfg.SpotifyClientSecret
		);

		if (!CreateProcessW(
			NULL,		// No module name (use command line)
			&path[0],   // Command line
			NULL,       // Process handle not inheritable
			NULL,       // Thread handle not inheritable
			FALSE,      // Set handle inheritance to FALSE
			0,          // No creation flags
			NULL,       // Use parent's environment block
			NULL,       // Use parent's starting directory 
			&relay_si,  // Pointer to STARTUPINFO structure
			&relay_pi)  // Pointer to PROCESS_INFORMATION structure
			)
		{
			printf("CreateProcess failed (%d).\n", GetLastError());
			stop_relay();

			return false;
		}

		return true;
	}

	void Browser::stop_relay()
	{
		// Check if handle is invalid or has allready been closed
		if (relay_pi.hProcess == NULL)
		{
			printf("Process handle invalid. Possibly allready been closed (%d).\n", GetLastError());
			return;
		}

		// Terminate Process
		if (!TerminateProcess(relay_pi.hProcess, 1))
		{
			printf("ExitProcess failed (%d).\n", GetLastError());
			return;
		}

		// Wait until child process exits.
		if (WaitForSingleObject(relay_pi.hProcess, INFINITE) == WAIT_FAILED)
		{
			printf("Wait for exit process failed(%d).\n", GetLastError());
			return;
		}

		// Close process and thread handles.
		if (!CloseHandle(relay_pi.hProcess))
		{
			printf("Cannot close process handle(%d).\n", GetLastError());
			return;
		}
		else
		{
			relay_pi.hProcess = NULL;
		}

		if (!CloseHandle(relay_pi.hThread))
		{
			printf("Cannot close thread handle (%d).\n", GetLastError());
			return;
		}
		else
		{
			relay_pi.hProcess = NULL;
		}
		return;
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
		ItemsCollection result =
		{
			{
				L"Artist1",
				L"Artist1"
			},
			{
				L"Artist2",
				L"Artist2"
			}
		};
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
