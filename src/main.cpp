#include "engine/window.hpp"
#include "engine/opengl.hpp"
#include "game/game.hpp"


void Parse(int argc, char** argv, bool* is_client, bool* is_server, std::string* address, unsigned* port, bool* verbose) {

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

	}

}
int main(int argc, char** argv)
{
	bool is_client = false;
	bool is_server = false;
	std::string address;
	unsigned port = 0;

	bool verbose = false;

	Parse(argc, argv, &is_client, &is_server, &address, &port, &verbose);



    game::instance().create();
    bool exit = false;
    do {
        exit = !game::instance().update();
    } while (!exit);
    game::instance().destroy();
    return 0;
}
