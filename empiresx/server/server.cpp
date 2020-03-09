/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/*
Dedicated headless server free software remake of
Age of Empires and Rise of Rome expansion
*/

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

#include "../string.hpp"
#include "../base/game.hpp"

uint16_t port = 25659;

namespace genie {

// dummy callbacks
void check_taunt(const std::string&) {}
void menu_lobby_stop_game(MenuLobby*) {}

namespace game {

class DedicatedGame;

// dummy draw. we don't do anything graphical, so this is just a nop.
void Particle::draw(int, int, unsigned) const {}
void Building::draw(int, int) const {}

void img_dim(Box2<float> &dim, int&, int&, unsigned, unsigned) {
	// we don't care about its dimensions, just that it represents some small area
	dim.w = dim.h = 10;
}

void worker_loop(DedicatedGame &game);

class DedicatedGame final : public Game {
	std::thread t_worker;
public:
	MultiplayerHost &cb;
	std::atomic<bool> running;

	DedicatedGame(const StartMatch &settings, MultiplayerHost &cb)
		//: Game(game::GameMode::multiplayer_host, nullptr, nullptr, settings), t_worker(worker_loop, std::ref(*this)), cb(cb) {}
		: Game(game::GameMode::multiplayer_host, nullptr, nullptr, settings), t_worker(), cb(cb) {
		world.populate(settings.slave_count);
		cb.set_gcb(this);
		t_worker = std::thread(worker_loop, std::ref(*this));
	}

	~DedicatedGame() {
		running.store(false);
		t_worker.join();
	}

	void new_player(const CreatePlayer &create) override {
		std::lock_guard<std::recursive_mutex> lock(mut);
		printf("new player %u: %s\n", create.id, create.str().c_str());
		players.emplace(create.id, create.str());
	}

	void assign_player(const AssignSlave& assign) override {
		std::lock_guard<std::recursive_mutex> lock(mut);
		assert(players.find(assign.to) != players.end());
		printf("assign user %u to %u: %s\n", assign.from, assign.to, players.find(assign.to)->name.c_str());
		usertbl.emplace(assign.from, assign.to);
	}

	void change_state(const GameState &state) override {
		std::lock_guard<std::recursive_mutex> lock(mut);
		printf("change gamestate to %u\n", (unsigned)state);
		this->state = state;
	}
};

void worker_loop(DedicatedGame &game) {
	game.running.store(true);
	auto start = std::chrono::high_resolution_clock::now();

	while (game.running.load() && !game.cb.try_start()) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(20ms);
	}

	while (game.running.load()) {
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end - start;
		game.step(diff.count());
		start = end;
	}
}

}

class DedicatedServer : public MultiplayerCallback {
	std::unique_ptr<game::DedicatedGame> game;
public:
	genie::MultiplayerHost mp;

	DedicatedServer() : mp(*this, "", port, true) {}

	void chat(const TextMsg &msg) override {}
	void chat(user_id from, const std::string &text) {}
	void join(JoinUser &usr) override {}
	void leave(user_id id) override {}

	void start(const StartMatch &match) override {
		game.reset(new game::DedicatedGame(match, mp));
	}
};

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
		genie::DedicatedServer server;
		std::string input;

		while (std::getline(std::cin, input)) {
			trim(input);

			if (input == "h" || input == "help") {
				std::cout <<
					"h(elp)/? - show this help\n"
					"q/quit   - fast shutdown server\n"
					"say      - broadcast message to clients\n"
					"start    - start new match\n" << std::endl;
			} else if (input == "q" || input == "quit") {
				break;
			} else if (input == "d") {
				server.mp.dump();
			} else if (starts_with(input, "say ")) {
				server.mp.chat(input.substr(strlen("say ")));
			} else if (input == "start") {
				server.mp.prepare_match();
			} else {
				std::cerr << "Unknown command. Type help for help" << std::endl;
			}
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
