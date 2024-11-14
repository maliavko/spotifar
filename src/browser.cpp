#include "stdafx.h"

#include "browser.hpp"
#include "config.hpp"


namespace spotifar
{
	auto& cfg = config::Opt;

	Browser::Browser()
	{
		start_relay();

		api.login();
	}

	Browser::~Browser()
	{
		stop_relay();
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
}
