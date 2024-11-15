#include "stdafx.h"

#include "spotify_http_relay.hpp"

using namespace spotifar;

int main(int argc, char* argv[])
{
	try
	{
		auto parser = utils::InputParser(argc, argv);

		SpotifyHttpRelay relay(
			parser.get_cmd_option<int>("--port"),
			parser.get_cmd_option<std::string>("--client-id"),
			parser.get_cmd_option<std::string>("--client-secret")
		);

		relay.listen();
	}
	catch (const utils::ConfigException& ex)
	{
		std::cerr << "A configuration exception occured: " << ex.what() << std::endl;
	}
	catch (const std::exception& ex)
	{
		std::cerr << "An unknown exception occured: " << ex.what() << std::endl;
	}
}
