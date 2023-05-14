#include "../server.hpp"

#include "../legacy/legacy.hpp"

#include <chrono>

#include <tracy/Tracy.hpp>

namespace aoe {

Entity *WorldView::try_get(IdPoolRef r) {
	return w.entities.try_get(r);
}

World::World()
	: m(), m_events(), t(), entities(), dirty_entities(), spawned_entities()
	, particles(), spawned_particles()
	, players(), player_achievements(), events_in(), events_out(), views()
	, resources_out(), s(nullptr), gameover(false), scn(), logic_gamespeed(1.0), running(false) {}

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

bool WorldView::try_convert(Entity &e, Entity &aggressor) {
	// TODO convert gradually with some randomness
	for (Player &p : w.players)
		p.entities.erase(e.ref);

	unsigned player = aggressor.playerid;

	e.playerid = player;
	w.players.at(player).entities.emplace(e.ref);

	return true;
}

void World::tick_entities() {
	ZoneScoped;

	WorldView wv(*this);
	died_entities.clear();

	for (auto &kv : entities) {
		Entity &ent = kv.second;

		if (is_building(ent.type))
			continue;

		bool alive = ent.is_alive();
		bool dirty = ent.tick(wv), tdirty = false;
		bool more = ent.imgtick(1);

		if (alive && !ent.is_alive())
			died_entities.emplace(ent.ref);

		switch (ent.state) {
			case EntityState::dying:
				if (!more) {
					ent.decay();
					dirty = true;
				}
				break;
			case EntityState::attack: {
				Entity *t = entities.try_get(ent.target_ref);
				if (!t || !t->is_alive()) {
					ent.task_cancel();
					dirty = true;
					break;
				}

				if (!more) {
					// prevent killing the entity twice. yes i've debug tested this can happen
					bool was_alive = t->is_alive();
					//printf("TODO attack (%u,%u) to (%u,%u)\n", ent.ref.first, ent.ref.second, ent.target_ref.first, ent.target_ref.second);
					if (was_alive) {
						dirty |= t->hit(wv, ent);
						dirty_entities.emplace(t->ref);
					}

					auto it = died_entities.find(t->ref);
					// if t died just now
					if (!t->is_alive()) {
						died_entities.emplace(t->ref);
						ent.task_cancel();
						dirty = true;
					}

					// if t died just now, remove from player
					if (!t->is_alive() && was_alive) {
						// update score but ensure entity isn't owned by gaia
						if (t->playerid != 0) {
							if (is_building(t->type))
								players[ent.playerid].killed_building();
							else
								players[ent.playerid].killed_unit();
						}
					}

					// TODO conversion is not detected!
				}
				break;
			}
		}

		if (dirty)
			dirty_entities.emplace(ent.ref);
	}

	// now iterate all died entities
	for (IdPoolRef ref : died_entities) {
		Entity &ent = entities.at(ref);
		players[ent.playerid].lost_entity(ref);
	}
}

/** Iterate all particles and remove those whose animation has ended. */
void World::tick_particles() {
	ZoneScoped;

	for (auto it = particles.begin(); it != particles.end();) {
		Particle &p = it->second;
		if (!p.imgtick(1))
			// no need to tell clients as they should already be able to detect this
			it = particles.erase(it);
		else
			++it;
	}
}

void World::tick_players() {
	ZoneScoped;

	// collect dead players, skip gaia cuz gaia is immortal
	for (unsigned i = 1; i < players.size(); ++i) {
		Player &p = players[i];

		// players may have additional rules to resign, but this always applies
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

	// collect surviving teams
	for (unsigned i = 1; i < players.size(); ++i) {
		Player &p = players[i];
		if (p.alive)
			alive_teams.emplace(p.init.team);
	}

	if (alive_teams.size() <= 1) {
		// game has ended. if no teams remain, gaia 'wins'. otherwise: remaining team
		stop(alive_teams.empty() ? 0 : *alive_teams.begin());
		return;
	}
}

void World::stop(unsigned team) {
	ZoneScoped;
	gameover = true;

	if (team)
		printf("gameover. winner: team %u\n", team);
	else
		puts("gameover. no winner");

	NetPkg pkg;
	pkg.set_gameover(team);
	s->broadcast(pkg);

	send_scores();
}

void World::tick() {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m);
	tick_entities();
	tick_particles();
	tick_players();
}

void World::save_scores() {
	ZoneScoped;
	player_achievements.clear();

	for (Player &p : players)
		player_achievements.emplace_back(p.get_score());
}

/** Process all events sent from peers to us. */
void World::pump_events() {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_events);

	for (WorldEvent &ev : events_in) {
		try {
			switch (ev.type) {
			case WorldEventType::entity_kill:
				if (running)
					entity_kill(ev);
				break;
			case WorldEventType::peer_cam_move:
				cam_move(ev);
				break;
			case WorldEventType::entity_task:
				if (running)
					entity_task(ev);
				break;
			case WorldEventType::gamespeed_control:
				gamespeed_control(ev);
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

/** Send any new changes back to the peers. */
void World::push_events() {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m);

	NetPkg pkg;

	push_entities();
	push_particles();

	for (WorldEvent &ev : events_out) {
		switch (ev.type) {
		case WorldEventType::gamespeed_control:
			push_gamespeed_control(ev);
			break;
		}
	}

	events_out.clear();

	push_scores();
	push_resources();
}

void World::push_entities() {
	ZoneScoped;
	NetPkg pkg;

	for (IdPoolRef ref : dirty_entities) {
		Entity *ent = entities.try_get(ref);
		if (!ent)
			continue;

		pkg.set_entity_update(*ent);
		s->broadcast(pkg);
	}

	dirty_entities.clear();

	for (IdPoolRef ref : spawned_entities) {
		Entity *ent = entities.try_get(ref);
		if (!ent)
			continue;

		pkg.set_entity_spawn(*ent);
		s->broadcast(pkg);
	}

	spawned_entities.clear();
}

void World::push_particles() {
	ZoneScoped;
	NetPkg pkg;

	for (IdPoolRef ref : spawned_particles) {
		Particle *p = particles.try_get(ref);
		if (!p)
			continue;

		pkg.particle_spawn(*p);
		s->broadcast(pkg);
	}

	spawned_particles.clear();
}

void World::push_scores() {
	ZoneScoped;
	NetPkg pkg;

	// check player achievements score
	assert(player_achievements.empty() || player_achievements.size() == players.size());

	// skip gaia
	for (unsigned i = 1; i < players.size(); ++i) {
		PlayerAchievements pa(players[i].get_score());

		if (i >= player_achievements.size() || player_achievements[i].score != pa.score) {
			if (i >= player_achievements.size())
				player_achievements.emplace_back(pa);

			pkg.set_player_score(i, pa);
			s->broadcast(pkg);
		}
	}
}

void World::push_resources() {
	ZoneScoped;
	NetPkg pkg;

	for (unsigned idx : resources_out) {
		Player &p = players.at(idx);
		pkg.set_resources(p.res);
		send_player(idx, pkg);
	}

	resources_out.clear();
}

void World::send_scores() {
	NetPkg pkg;

	for (unsigned i = 1; i < players.size(); ++i) {
		pkg.set_player_score(i, players[i].get_score());
		s->broadcast(pkg);
	}
}

void World::send_resources() {
	NetPkg pkg;

	for (unsigned i = 1; i < players.size(); ++i) {
		Player &p = players[i];

		pkg.set_resources(p.res);
		send_player(i, pkg);
	}
}

void World::push_gamespeed_control(WorldEvent &ev) {
	NetGamespeedControl ctl = std::get<NetGamespeedControl>(ev.data);

	switch (ctl.type) {
	case NetGamespeedType::toggle_pause: {
		NetPkg pkg;

		if (running)
			pkg.set_gamespeed(NetGamespeedType::unpause);
		else
			pkg.set_gamespeed(NetGamespeedType::pause);

		s->broadcast(pkg);
		break;
	}
	default:
		printf("%s: gamespeed control type %u\n", __func__, (unsigned)ctl.type);
		break;
	}
}

void World::cam_move(WorldEvent &ev) {
	ZoneScoped;
	EventCameraMove move(std::get<EventCameraMove>(ev.data));
	views.at(move.ref) = move.cam;
}

void World::gamespeed_control(WorldEvent &ev) {
	ZoneScoped;
	NetGamespeedControl ctl(std::get<NetGamespeedControl>(ev.data));

	switch (ctl.type) {
	case NetGamespeedType::toggle_pause:
		this->running = !this->running;
		events_out.emplace_back(ev);
		break;
	case NetGamespeedType::increase:
		// disallow changing gamespeed while paused
		if (this->running)
			this->logic_gamespeed = std::clamp(this->logic_gamespeed + World::gamespeed_step, World::gamespeed_min, World::gamespeed_max);
		break;
	case NetGamespeedType::decrease:
		// disallow changing gamespeed while paused
		if (this->running)
			this->logic_gamespeed = std::clamp(this->logic_gamespeed - World::gamespeed_step, World::gamespeed_min, World::gamespeed_max);
		break;
	}
}

void World::entity_kill(WorldEvent &ev) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m);
	IdPoolRef ref = std::get<IdPoolRef>(ev.data);

	// TODO move this to an entity_die function
	// entity_die is like it does its proper dying animation (opt. with particles)
	// entity_kill is like when it has to be removed completely (e.g. decaying ended, resource depleted)
	Entity *ent = entities.try_get(ref);
	if (ent && !is_building(ent->type)) {
		if (is_resource(ent->type)) {
			// TODO
		} else {
			if (ent->die()) {
				players[ent->playerid].lost_entity(ref);
				dirty_entities.emplace(ent->ref);
			}
			return;
		}
	}

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

void World::entity_task(WorldEvent &ev) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m);
	EntityTask task = std::get<EntityTask>(ev.data);

	Entity *ent = entities.try_get(task.ref1);
	if (!ent)
		return;

	// check if the event has been created by a peer. if true, check permissions
	auto idx = ref2idx(ev.src);
	if (idx.has_value()) {
		// check permissions
		unsigned playerid = idx.value();

		if (ent->playerid != playerid)
			return; // permission denied
	}

	switch (task.type) {
		case EntityTaskType::move:
			if (ent->task_move(task.x, task.y)) {
				dirty_entities.emplace(ent->ref);
				spawn_particle(ParticleType::moveto, task.x, task.y);
			}
			break;
		case EntityTaskType::infer: {
			// TODO determine task type. for now always assume attack
			Entity *target = entities.try_get(task.ref2);
			if (!target)
				break;

			if (ent->task_attack(*target))
				dirty_entities.emplace(ent->ref);
			break;
		}
		case EntityTaskType::train_unit: {
			assert(task.info_type == (unsigned)EntityIconType::unit);
			EntityType train = (EntityType)task.info_value;

			// TODO extract resources cost from entity info
			Resources cost(0, 50, 0, 0);
			bool can_train = true;

			if (idx.has_value()) {
				unsigned playerid = idx.value();
				Player &p = players.at(playerid);
				can_train = p.res.can_afford(cost) && ent->task_train_unit(train);
				if (can_train) {
					resources_out.emplace(playerid);
					p.res -= cost;
				}
			} else {
				can_train = ent->task_train_unit(train);
			}

			if (can_train) {
				// TODO add to build queue stuff
				spawn_unit(train, ent->playerid, ent->x + 2, ent->y + 2, fmodf(rand(), 360));
			}
			break;
		}
		default:
			fprintf(stderr, "%s: unknown entity task type %u\n", __func__, (unsigned)task.type);
			break;
	}
}

bool World::single_team() const noexcept {
	// skip gaia as gaia is not part of any team
	unsigned team = scn.players.at(1).team;

	for (unsigned i = 2; i < scn.players.size(); ++i)
		if (scn.players[i].team != team)
			return false;

	return true;
}

void World::send_player(unsigned i, NetPkg &pkg) {
	ZoneScoped;

	for (auto kv : scn.owners) {
		if (kv.second != i)
			continue;

		const Peer *p = s->try_peer(kv.first);
		if (p)
			s->send(*p, pkg);
	}
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
	add_unit(t, player, x, y, fmodf(rand(), 360));
}

void World::add_unit(EntityType t, unsigned player, float x, float y, float angle, EntityState state) {
	ZoneScoped;
	assert(!is_building(t) && !is_resource(t) && player < MAX_PLAYERS);
	auto p = entities.emplace(t, player, x, y, angle, state);
	assert(p.second);
	players.at(player).entities.emplace(p.first->first);
}

void World::add_resource(EntityType t, float x, float y) {
	ZoneScoped;
	assert(is_resource(t));
	// TODO add resource values
	entities.emplace(t, 0, x, y, 0, EntityState::alive);
}

void World::spawn_unit(EntityType t, unsigned player, float x, float y) {
	spawn_unit(t, player, x, y, fmodf(rand(), 360));
}

void World::spawn_unit(EntityType t, unsigned player, float x, float y, float angle) {
	ZoneScoped;
	assert(!is_building(t) && !is_resource(t) && player < MAX_PLAYERS);

	auto p = entities.emplace(t, player, x, y, angle, EntityState::alive);
	assert(p.second);

	IdPoolRef ref = p.first->first;
	players.at(player).entities.emplace(ref);
	spawned_entities.emplace(ref);
}

void World::spawn_particle(ParticleType t, float x, float y)
{
	ZoneScoped;

	auto p = particles.emplace(t, x, y, 0);
	assert(p.second);

	IdPoolRef ref = p.first->first;
	spawned_particles.emplace(ref);
}

void World::create_entities() {
	ZoneScoped;

	entities.clear();
	for (unsigned i = 1; i < players.size(); ++i) {
		add_building(EntityType::town_center, i, 2, 1 + 3 * i);
		add_building(EntityType::barracks, i, 2 + 2 * 3, 1 + 3 * i);

		add_unit(EntityType::villager, i, 5, 1 + 3 * i);
		add_unit(EntityType::villager, i, 5, 2 + 3 * i);
		add_unit(EntityType::villager, i, 6, 1 + 3 * i);// , 0, EntityState::attack);
		add_unit(EntityType::villager, i, 6, 2 + 3 * i);

		add_unit(EntityType::melee1, i, 2 + 3 * 3, 1 + 3 * i);
		add_unit(EntityType::melee1, i, 2 + 3 * 3, 2 + 3 * i);
	}

	add_unit(EntityType::priest, 0, 2.5, 1);
	add_unit(EntityType::priest, 0, 3.5, 1);

	add_resource(EntityType::berries, 0, 0);
	add_resource(EntityType::berries, 0, 1);
	add_resource(EntityType::berries, 1, 0);
	add_resource(EntityType::berries, 1, 1);

	add_resource(EntityType::desert_tree1, 2, 0);
	add_resource(EntityType::desert_tree2, 3, 0);
	add_resource(EntityType::desert_tree3, 4, 0);
	add_resource(EntityType::desert_tree4, 5, 0);
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
	// TODO this is buggy if the map is too small...
	unsigned w = 16, h = 16;
	NetTerrainMod tm(fetch_terrain(0, 0, w, h));

	pkg.set_terrain_mod(tm);
	s->broadcast(pkg);

	this->running = true;

	// start!
	pkg.set_start_game();
	s->broadcast(pkg);

	send_resources();
	send_scores();
}

void World::send_gameticks(unsigned n) {
	ZoneScoped;
	if (!n)
		return;

	NetPkg pkg;
	pkg.set_gameticks(n);
	s->broadcast(pkg);
}

std::optional<unsigned> World::ref2idx(IdPoolRef ref) const noexcept {
	for (auto kv : scn.owners)
		if (kv.first == ref)
			return kv.second;

	return std::nullopt;
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

		if (running) {
			if (!gameover)
				send_gameticks(steps);

			save_scores();

			// do steps
			for (; steps && !gameover; --steps)
				tick();
		}

		push_events();

		dt = fmod(dt, interval);

		unsigned us = 0;

		if (!steps)
			us = (unsigned)(interval * 1000 * 1000);

		if (us > 500)
			std::this_thread::sleep_for(std::chrono::microseconds(us));
	}
}

}
