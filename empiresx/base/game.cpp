/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include "game.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include <inttypes.h>

#include "../string.hpp"

#include <string>
#include <map>

namespace genie {

extern void menu_lobby_stop_game(MenuLobby *lobby);

void host_start(MultiplayerHost& host) {
	host.eventloop();
}

void client_start(MultiplayerClient& client) {
	puts("start client");
	client.eventloop();
}

bool operator<(const Slave &lhs, const Slave &rhs) {
	return lhs.fd < rhs.fd;
}

bool operator<(const Peer &lhs, const Peer &rhs) {
	return lhs.id < rhs.id;
}

Slave::Slave(sockfd fd) : fd(fd), id(0), pid(0), name() {}
Slave::Slave(sockfd fd, user_id id) : fd(fd), id(id), pid(0), name() {}
Slave::Slave(const std::string &name) : fd(INVALID_SOCKET), id(0), name(name) {}

Multiplayer::Multiplayer(MultiplayerCallback &cb, const std::string &name, uint16_t port)
	: net(), name(name), port(port), t_worker(), mut(), cb(cb), gcb(nullptr), invalidated(false), self(0) {}

void MultiplayerClient::set_gcb(game::GameCallback *gcb, uint16_t slave_count, uint16_t prng_next) {
	std::lock_guard<std::recursive_mutex> lock(mut);
	this->gcb = gcb;

	Command cmd = Command::ready(slave_count, prng_next);
	sock.send(cmd, false);
}

MultiplayerHost::MultiplayerHost(MultiplayerCallback &cb, const std::string &name, uint16_t port, bool dedicated)
	: Multiplayer(cb, name, port), sock(port), slaves(), idmod(1), ready_confirms(0), dedicated(dedicated)
{
	puts("start host");
	srand((unsigned)time(NULL));
	// claim slot for server itself: id == 0 is used for that purpose
	slaves.emplace(name);
	t_worker = std::thread(host_start, std::ref(*this));
}

MultiplayerHost::~MultiplayerHost() {
	puts("closing host");
	sock.close();
#if linux
	pthread_cancel(t_worker.native_handle());
#endif
	t_worker.join();
	puts("host stopped");
}

Slave &MultiplayerHost::slave(sockfd fd) {
	auto search = slaves.find(fd);
	if (search == slaves.end())
		throw std::runtime_error(std::string("bad slave fd: ") + std::to_string(fd));

	return const_cast<Slave&>(*search);
}

void MultiplayerHost::eventloop() {
	printf("host started on 127.0.0.1:%" PRIu16 "\n", port);
	sock.eventloop(*this);
}

void MultiplayerHost::event_process(sockfd fd, Command &cmd) {
	// we always need the lock, because cb access must be thread-safe
	std::lock_guard<std::recursive_mutex> lock(mut);

	switch ((CmdType)cmd.type) {
	case CmdType::text:
		{
			auto str = cmd.text();
			Slave &s = slave(fd);
			if (str.from != s.id)
				fprintf(stderr, "bad id for %s, expected %u, got %u\n", s.name.c_str(), s.id, str.from);
			str.from = s.id;
			cb.chat(str);
		}
		sock.broadcast(*this, cmd);
		break;
	case CmdType::join:
		{
			auto join = cmd.join();

			Slave &s = slave(fd);
			s.name = join.nick().c_str();

#if windows
			printf("%llu joins as %u: %s\n", fd, s.id, join.nick().c_str());
#else
			printf("%d joins as %u: %s\n", fd, s.id, join.nick().c_str());
#endif

			Command cmd = Command::join(s.id, s.name);
			cmd.hton();

			// always send back first to the slave it came from
			sock.broadcast(*this, cmd, fd, true);

			// send all joined slaves to new client
			for (auto &x : slaves) {
				// ignore special slave and client itself
				if ((x.id == 0 && x.name.empty()) || x.id == s.id)
					continue;
				Command cmd = Command::join(x.id, x.name);
				sock.push(fd, cmd, false);
			}

			// all bookkeeping is up-to-date, update to real ID and notify callback
			join.id = s.id;
			cb.join(join);
		}
		break;
	case CmdType::ready:
		// ensure expected settings match
		if (expected_settings != cmd.ready()) {
			Slave &s = slave(fd);
			fprintf(stderr, "bad ready settings for slave %u: %s\n", s.id, s.name.c_str());
			sock.close();
			cb.leave(s.id);
		}
		--ready_confirms;
		break;
	}
}

void MultiplayerHost::incoming(pollev &ev) {
	std::lock_guard<std::recursive_mutex> lock(mut);
	// disallow id 0 as slave, because this is always the host itself
	if (idmod == 0)
		++idmod;

	slaves.emplace(pollfd(ev), idmod++);
}

void MultiplayerHost::removepeer(sockfd fd) {
	std::lock_guard<std::recursive_mutex> lock(mut);

	Slave &s = slave(fd);
	user_id leave = s.id;
	assert(leave);
	printf("%s has left\n", s.name.c_str());
	cb.leave(leave);

	slaves.erase(fd);

	Command cmd = Command::leave(leave);
	sock.broadcast(*this, cmd, false, true);
}

void MultiplayerHost::shutdown() {
	puts("host shutdown");
	std::lock_guard<std::recursive_mutex> lock(mut);
	slaves.clear();
}

void MultiplayerHost::dump() {
	std::lock_guard<std::recursive_mutex> lock(mut);
	printf("slaves: %lu\n", (long unsigned)slaves.size());

	for (auto &x : slaves)
		printf("%u %s\n", x.id, x.name.c_str());
}

void MultiplayerHost::set_gcb(game::GameCallback *gcb) {
	std::lock_guard<std::recursive_mutex> lock(mut);
	this->gcb = gcb;
}

bool MultiplayerHost::try_start() {
	std::lock_guard<std::recursive_mutex> lock(mut);
	if (ready_confirms) {
		//printf("need %u more confirms\n", ready_confirms);
		return false;
	}

	assert(gcb);

	// create players
	player_id pid = 0;

	for (auto &x : slaves) {
		if (x.id == 0 && dedicated)
			continue;

		// announce player to slaves
		Command create = Command::create(const_cast<Slave&>(x).pid = pid++, x.name);
		gcb->new_player(create.data.create);
		sock.broadcast(*this, create);

		// assign slave to player
		Command assign = Command::assign(x.id, x.pid);
		gcb->assign_player(assign.data.assign);
		sock.broadcast(*this, assign);
	}

	// TODO create random stuff on terrain

	// TODO announce all slaves to start the game

	return true;
}


bool MultiplayerHost::chat(const std::string &str, bool send) {
	Command txt = Command::text(0, str);

	if (send)
		sock.broadcast(*this, txt);

	std::lock_guard<std::recursive_mutex> lock(mut);
	cb.chat(txt.text());
	return true;
}

void MultiplayerHost::prepare_match() {
	std::lock_guard<std::recursive_mutex> lock(mut);
	unsigned count = (unsigned)slaves.size();
	ready_confirms = count - 1;
	// headless server does not announce 'hidden' slave
	if (dedicated)
		--count;
	StartMatch settings = StartMatch::random(count, count);
	Command start = Command::start(settings);

	// ignore any new clients: the match has already started at this point
	sock.accept(false);
	expected_settings.slave_count = count;
	sock.broadcast(*this, start, false);
	cb.start(settings);
}

MultiplayerClient::MultiplayerClient(MultiplayerCallback &cb, const std::string &name, uint32_t addr, uint16_t port)
	: Multiplayer(cb, name, port), sock(port), addr(addr), activated(false), peers()
{
	sock.reuse();

	t_worker = std::thread(client_start, std::ref(*this));
}

MultiplayerClient::~MultiplayerClient() {
	puts("closing client");
	// NOTE sock.close() has to be in both bodies, because its behavior will be slightly different!
	// XXX figure out how to force a memory barrier between joinable and close or verify that the generated assembly is correct
	if (t_worker.joinable()) {
		sock.close();
#if linux
		pthread_cancel(t_worker.native_handle());
#endif
		t_worker.join();
	} else {
		sock.close();
	}
	puts("client stopped");
}

void MultiplayerClient::eventloop() {
	bool connected = false;

	// FIXME improve connection establishment tolerance
	/**
	 * het idee is (de volgende punten gecombineerd):
	 * - probeer een X aantal keer te verbinden, maar altijd minstens 2 keer
	 * - als een connect poging mislukt, kijk hoeveel tijd er sinds connect verstreken is.
	 *   als het minder dan 500ms was, wacht totdat er ongeveer intotaal 500ms is verstreken
	 * - geef pas op als er minimaal Y ms zijn verstreken
	 */

	for (unsigned tries = 3; tries && !connected; --tries) {
		int err;

		if (!(err = sock.connect(addr, true))) {
			connected = true;
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	if (!connected) {
		chat("Failed to connect", false);
		return;
	}

	chat("Connected to server", false);

	// send desired nickname
	Command cmd = Command::join(0, name);
	sock.send(cmd, false);

	// main loop
	for (activated.store(true); activated.load();) {
		Command cmd;

		if (sock.recv(cmd)) {
			activated.store(false);
			chat("Server stopped", false);
			continue;
		}

		// we always need the lock, because cb access must be thread-safe
		std::lock_guard<std::recursive_mutex> lock(mut);

		switch ((CmdType)cmd.type) {
		case CmdType::text:
			cb.chat(cmd.text());
			break;
		case CmdType::join:
			{
				JoinUser usr = cmd.join();
				const std::string &s = usr.nick();

				if (self == 0) {
					self = usr.id;
					name = s;
					printf("joined as %u: %s\n", usr.id, s.c_str());
				}

				peers.emplace(std::piecewise_construct, std::forward_as_tuple(usr.id), std::forward_as_tuple(usr.id, s));
				cb.join(usr);
			}
			break;
		case CmdType::leave:
			{
				user_id leave = cmd.data.leave;

				if (leave == self) {
					fputs("we got kicked!\n", stderr);
					cb.chat(0, "You are kicked from the server");
					activated.store(false);
					continue;
				}

				auto search = peers.find(leave);

				if (search != peers.end())
					cb.leave(leave);

				peers.erase(leave);
			}
			break;
		case CmdType::start:
			cb.start(cmd.data.start);
			break;
		default:
			fprintf(stderr, "communication error: unknown type %u\n", cmd.type);
			cb.chat(0, "Communication error");
			activated.store(false);
			continue;
		case CmdType::create:
			assert(gcb);
			gcb->new_player(cmd.data.create);
			break;
		case CmdType::assign:
			gcb->assign_player(cmd.data.assign);
			break;
		}
	}

	sock.close();
}

void Multiplayer::dispose() {
	std::lock_guard<std::recursive_mutex> lock(mut);
	invalidated = true;
}

#pragma warning(push)
#pragma warning(disable: 26117)

// XXX broken MSVC lock balancing/bogus lock balancing warning?
bool MultiplayerClient::chat(const std::string &str, bool send) {
	std::lock_guard<std::recursive_mutex> lock(mut);

	if (invalidated)
		return false;

	if (!send || !activated.load()) {
		cb.chat(0, send ? "Not connected to server" : str);
		return false; // should be balanced, since dtor must be called...
	}

	Command cmd = Command::text(self, str);
	sock.send(cmd, false);
	return true;
}

#pragma warning(pop)

namespace game {

Map::Map(LCG &lcg, const StartMatch &settings) : w(settings.map_w), h(settings.map_h), tiles(new uint8_t[h * w]), heights(new uint8_t[h * w]) {
	printf("create %ux%u tiles\n", w, h);

	for (unsigned y = 0; y < h; ++y)
		for (unsigned x = 0; x < w; ++x)
			tiles[y * w + x] = lcg.next() % ((unsigned)TileId::FLAT9 + 1);

	// TODO support heightmaps
	memset(heights.get(), 0, w * h);
}

Player::Player(player_id id) : Player(id, "") {}

Player::Player(player_id id, const std::string &name)
	: id(id), state(PlayerState::active), cheats((PlayerCheat)0), ai(0), name(name) {}

bool operator<(const Player &lhs, const Player &rhs) {
	return lhs.id < rhs.id;
}

Game::Game(GameMode mode, MenuLobby *lobby, Multiplayer *mp, const StartMatch &settings)
	: mp(mp), lobby(lobby), mode(mode), state(GameState::init), lcg(LCG::ansi_c(settings.seed))
	, settings(settings), players(), usertbl(), mut(), world(lcg, settings, mode != GameMode::multiplayer_client)
	, ticks_per_second(50), tick_interval(1.0 / ticks_per_second), tick_timer(0) {}

Game::~Game() {
	if (lobby)
		menu_lobby_stop_game(lobby);
}

void Game::tick(unsigned n) {
	printf("do %u ticks\n", n);
}

void Game::step(unsigned ms) {
	std::lock_guard<std::recursive_mutex> lock(mut);

	if (state != GameState::running)
		return;

	tick_timer += ms / 1000.0;
	if (tick_timer >= tick_interval) {
		tick((unsigned)(tick_timer / tick_interval));
		tick_timer = fmod(tick_timer, tick_interval);
	}
}

void Game::step(double sec) {
	std::lock_guard<std::recursive_mutex> lock(mut);

	if (state != GameState::running)
		return;

	tick_timer += sec;
	if (tick_timer >= tick_interval) {
		tick((unsigned)(tick_timer / tick_interval));
		tick_timer = fmod(tick_timer, tick_interval);
	}
}

}
}
