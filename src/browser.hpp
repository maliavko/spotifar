#ifndef CONTROLLER_HPP_5BAFAEB4_0F9B_411A_8C57_D6A34B0412FC
#define CONTROLLER_HPP_5BAFAEB4_0F9B_411A_8C57_D6A34B0412FC

#pragma once

#include "spotify.hpp"
#include "data_provider.hpp"

#include <windows.h>

namespace spotifar
{
	class Browser
	{
	public:
		Browser();
		~Browser();

	protected:
		bool start_relay();
		void WINAPI stop_relay();

	private:
		SpotifyRelayApi api;
		DataProvider data;

		STARTUPINFO relay_si;
		PROCESS_INFORMATION relay_pi;
	};
}


#endif //CONTROLLER_HPP_5BAFAEB4_0F9B_411A_8C57_D6A34B0412FC
