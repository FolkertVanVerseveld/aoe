/*
Dedicated headless server free software remake of
Age of Empires and Rise of Rome expansion
*/

#include <iostream>
#include <string>

#include "../string.hpp"
#include "../base/game.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

uint16_t port = 25659;

namespace genie {

void check_taunt(const std::string&) {}

}

int main(int argc, char **argv) {
	if (argc == 2) {
		port = atoi(argv[1]);
		if (port < 1 || port > UINT16_MAX) {
			fprintf(stderr, "%s: invalid port number or port out of range\n", argv[1]);
			return 1;
		}
	}

	try {
		genie::MultiplayerHost mp(port);
		std::string input;

		while (std::getline(std::cin, input)) {
			trim(input);

			if (input == "h" || input == "help") {
				std::cout <<
					"h(elp)/? - show this help\n"
					"q/quit   - fast shutdown server\n"
					"say      - broadcast message to clients" << std::endl;
			} else if (input == "q" || input == "quit") {
				break;
			} else if (input == "say ") {
				mp.chat(input.substr(strlen("say ")));
			} else {
				std::cerr << "Unknown command. Type help for help" << std::endl;
			}
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
