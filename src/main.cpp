#include "engine/window.hpp"
#include "engine/opengl.hpp"
#include "game/game.hpp"

#include "networking/server.hpp"
#include "networking/client.hpp"
#include "networking/networking.hpp"
#include "networking/utils.hpp"


void Parse(int argc, char** argv, bool* is_client, bool* is_server, std::string* address, uint16_t* port, bool* verbose, bool* is_solo) {

	for (int i = 0; i < argc; i++) {// for each argument we find, parse it

		if (strcmp("--client", argv[i]) == 0) {
			*is_client = true;
		}

		if (strcmp("--server", argv[i]) == 0) {
			*is_server = true;
		}

		if (strcmp("--address", argv[i]) == 0) {
			*address = (argv[i + 1]);
		}

		if (strcmp("--port", argv[i]) == 0) {
			*port = atoi(argv[i + 1]);
		}

		if (strcmp("--verbose", argv[i]) == 0) {
			*verbose = true;
		}
		
		if (strcmp("--solo", argv[i]) == 0) {
			*is_solo = true;
		}

	}

}
int main(int argc, char** argv)
{
	bool is_client = false;
	bool is_server = false;
	bool is_solo = false;
	std::string address;
	uint16_t port = 0;

	bool verbose = false;

	Parse(argc, argv, &is_client, &is_server, &address, &port, &verbose, &is_solo);

	game::instance().create(is_server, address, port, verbose, is_solo);

	bool exit = false;
	do
	{
		exit = !game::instance().update();
	} while (!exit);
	
    game::instance().destroy();
    return 0;
}
