#include "../server.hpp"

#include "../legacy.hpp"

#include <chrono>

#include <tracy/Tracy.hpp>

namespace aoe {

World::World() : m(), m_events(), t(), entities(), players(), events_in(), views(), s(nullptr), gameover(false), scn(), logic_gamespeed(1.0) {}

void World::load_scn(const ScenarioSettings &scn) {
	ZoneScoped;

	// TODO filter bogus settings
	this->scn.width = scn.width;
	this->scn.height = scn.height;
	this->scn.popcap = scn.popcap;
	this->scn.age = scn.age;
	this->scn.seed = scn.seed;
	this->scn.villagers = scn.villagers;

	this->scn.res = scn.res;

	this->scn.fixed_start = scn.fixed_start;
	this->scn.explored = scn.explored;
	this->scn.all_technologies = scn.all_technologies;
	this->scn.cheating = scn.cheating;
	this->scn.square = scn.square;
	this->scn.wrap = scn.wrap;

	t.resize(this->scn.width, this->scn.height, this->scn.seed, this->scn.players.size(), this->scn.wrap);
}

void World::create_terrain() {
	ZoneScoped;
	this->t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	this->t.generate();
}

NetTerrainMod World::fetch_terrain(int x, int y, unsigned &w, unsigned &h) {
	ZoneScoped;
	assert(x >= 0 && y >= 0);

	NetTerrainMod tm;
	t.fetch(tm.tiles, tm.hmap, 0, 0, w, h);

	tm.x = x; tm.y = y; tm.w = w; tm.h = h;

	return tm;
}

void World::tick_entities() {
	ZoneScoped;

	for (auto kv: entities) {
		Entity &ent = kv.second;

		if (is_building(ent.type))
			continue;

		++ent.subimage;
	}
}

void World::tick_players() {
	ZoneScoped;

	// collect dead players, skip gaia cuz gaia is immortal
	for (unsigned i = 1; i < players.size(); ++i) {
		Player &p = players[i];

		if (!p.alive || !p.entities.empty())
			continue;

		p.alive = false;
		NetPkg pkg;
		pkg.set_player_died(i);
		s->broadcast(pkg);
	}

	if (players.size() <= 2)
		return; // no game over condition

	std::set<unsigned> alive_teams;

	// check surviving teams
	for (unsigned i = 1; i < players.size(); ++i) {
		Player &p = players[i];
		if (p.alive)
			alive_teams.emplace(p.init.team);
	}

	if (alive_teams.size() <= 1) {
		// game has ended
		stop();
		return;
	}
}

void World::stop() {
	ZoneScoped;
	gameover = true;

	NetPkg pkg;
	pkg.set_gameover();
	s->broadcast(pkg);
}

void World::tick() {
	ZoneScoped;
	tick_entities();
	tick_players();
}

void World::pump_events() {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_events);

	for (WorldEvent &ev : events_in) {
		// TODO stub

		try {
			switch (ev.type) {
			case WorldEventType::entity_kill:
				entity_kill(ev);
				break;
			case WorldEventType::peer_cam_move:
				cam_move(ev);
				break;
			default:
				printf("%s: todo: process event: %u\n", __func__, (unsigned)ev.type);
				break;
			}
		} catch (const std::runtime_error &e) {
			fprintf(stderr, "%s: bad event: %u\n", __func__, (unsigned)ev.type);
		}
	}

	events_in.clear();
}

void World::cam_move(WorldEvent &ev) {
	ZoneScoped;
	EventCameraMove move(std::get<EventCameraMove>(ev.data));
	views.at(move.ref) = move.cam;
}

void World::entity_kill(WorldEvent &ev) {
	ZoneScoped;
	IdPoolRef ref = std::get<IdPoolRef>(ev.data);

	// TODO add client info that sent kill command
	// TODO check if player is allowed to kill this entity
	if (entities.try_invalidate(ref)) {
		for (Player &p : players)
			p.entities.erase(ref);

		NetPkg pkg;
		pkg.set_entity_kill(ref);
		s->broadcast(pkg);
	}
}

bool World::single_team() const noexcept {
	unsigned team = scn.players.at(1).team;

	for (unsigned i = 2; i < scn.players.size(); ++i)
		if (scn.players[i].team != team)
			return false;

	return true;
}

void World::create_players() {
	ZoneScoped;

	NetPkg pkg;

	// force Gaia as special player
	PlayerSetting &gaia = scn.players.at(0);
	gaia.ai = true;
	gaia.team = 0;

	bool one_team = single_team();

	// sanitize players: change team if one_team and check civ and name
	for (unsigned i = 0; i < scn.players.size(); ++i) {
		PlayerSetting &p = scn.players[i];

		if (one_team)
			// change team
			p.team = i;

		p.res = scn.res;

		// if player has no name, try find an owner that has one
		if (p.name.empty()) {
			unsigned owners = 0;
			std::string alias;

			for (auto kv : scn.owners) {
				if (kv.second == i) {
					++owners;
					alias = s->get_ci(kv.first).username;
				}
			}

			if (owners == 1) {
				p.name = alias;
			} else {
				p.name = "Oerkneus de Eerste";

				if (p.civ >= 0 && p.civ < s->civs.size()) {
					auto &names = s->civs[s->civnames[p.civ]];
					p.name = names[rand() % names.size()];
				}
			}
		}

		pkg.set_player_name(i, p.name);
		s->broadcast(pkg);
		pkg.set_player_civ(i, p.civ);
		s->broadcast(pkg);
		pkg.set_player_team(i, p.team);
		s->broadcast(pkg);
	}

	size_t size = (size_t)scn.width * scn.height;
	players.clear();
	for (const PlayerSetting &ps : scn.players)
		players.emplace_back(ps, size);

	// create player views
	for (auto kv : s->peers)
		views.emplace(kv.second.ref, NetCamSet());
		// TODO send view to client
}

void World::add_building(EntityType t, unsigned player, int x, int y) {
	ZoneScoped;
	assert(is_building(t) && player < MAX_PLAYERS);
	auto p = entities.emplace(t, player, x, y);
	assert(p.second);
	players.at(player).entities.emplace(p.first->first);
}

void World::add_unit(EntityType t, unsigned player, float x, float y) {
	ZoneScoped;
	assert(!is_building(t) && player < MAX_PLAYERS);
	auto p = entities.emplace(t, player, x, y);
	assert(p.second);
	players.at(player).entities.emplace(p.first->first);
}

void World::create_entities() {
	ZoneScoped;

	entities.clear();
	for (unsigned i = 1; i < players.size(); ++i) {
		add_building(EntityType::town_center, i, 2, 1 + 3 * i);
		add_building(EntityType::barracks, i, 2 + 2 * 3, 1 + 4 * i);

		add_unit(EntityType::villager, i, 5, 1 + 3 * i);
	}

	add_unit(EntityType::bird1, 0, 11, 6);
	add_unit(EntityType::bird1, 0, 12, 6);
}

void World::startup() {
	ZoneScoped;

	NetPkg pkg;

	// announce server is starting
	pkg.set_start_game();
	s->broadcast(pkg);

	pkg.set_scn_vars(scn);
	s->broadcast(pkg);

	create_terrain();
	create_players();
	create_entities();

	// now send all entities to each client
	for (auto &kv : entities) {
		pkg.set_entity_add(kv.second);
		// TODO only send to clients that can see this entity
		s->broadcast(pkg);
	}

	// send initial terrain chunk
	unsigned w = 16, h = 16;
	NetTerrainMod tm(fetch_terrain(0, 0, w, h));

	pkg.set_terrain_mod(tm);
	s->broadcast(pkg);

	// start!
	pkg.set_start_game();
	s->broadcast(pkg);
}

void World::send_gameticks(unsigned n) {
	ZoneScoped;
	if (!n)
		return;

	NetPkg pkg;
	pkg.set_gameticks(n);
	s->broadcast(pkg);
}

void World::eventloop(Server &s) {
	ZoneScoped;

	this->s = &s;
	startup();

	auto last = std::chrono::steady_clock::now();
	double dt = 0;

	while (s.m_running.load() && !gameover) {
		// recompute as logic_gamespeed may change
		double interval_inv = logic_gamespeed * DEFAULT_TICKS_PER_SECOND;
		double interval = 1 / std::max(0.01, interval_inv);

		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - last;
		last = now;
		dt += elapsed.count();

		size_t steps = (size_t)(dt * interval_inv);

		pump_events();

		if (!gameover)
			send_gameticks(steps);

		// do steps
		for (; steps && !gameover; --steps)
			tick();

		dt = fmod(dt, interval);

		unsigned us = 0;

		if (!steps)
			us = (unsigned)(interval * 1000 * 1000);

		if (us > 500)
			std::this_thread::sleep_for(std::chrono::microseconds(us));
	}
}

}
