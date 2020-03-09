/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "world.hpp"

#include "net.hpp"
#include "drs.hpp"
#include "random.hpp"

#include <cmath>
#include <inttypes.h>

namespace genie {

namespace game {

GatherStatus Resource::gather(Resource &dest, unsigned amount) {
	if (dest.type != type)
		return GatherStatus::incompatible;

	if (amount >= this->amount) {
		dest.amount += this->amount;
		this->amount = 0;
		return GatherStatus::depleted;
	}

	dest.amount += amount;
	this->amount -= amount;
	return GatherStatus::ok;
}

unsigned particle_id_counter = 1;

const unsigned res_hp[] = {
	40,
	1,
	1,
	1,
};

// FIXME verify these
const unsigned res_size[] = {
	14,
	16,
	40,
	40,
};

const unsigned res_anim[] = {
	463,
	240,
	481,
	622,
};

// FIXME verify these
const unsigned res_amount[] = {
	40,
	150,
	400,
	250,
};

StaticResource::StaticResource(Map &map, const Box2<float> &pos, ResourceType type, unsigned res_anim, unsigned image)
	: Particle(map, pos, res_anim, image)
	, Resource(type, res_amount[(unsigned)type]) {}

World::World(LCG &lcg, const StartMatch &settings, bool host)
	: map(lcg, settings), lcg(lcg), host(host)
	, static_res()
	//, tiled_objects(Vector2<int>(ispow2(settings.map_w) ? settings.map_w : nextpow2(settings.map_w), ispow2(settings.map_h) ? settings.map_h : nextpow2(settings.map_h)))
	//, movable_objects(Vector2<float>(static_cast<float>(settings.map_w), static_cast<float>(settings.map_h)))
{
}

void World::populate(unsigned players) {
	size_t tiles = static_cast<size_t>(map.w) * map.h;
	size_t trees = static_cast<size_t>(round(pow(static_cast<double>(tiles), 0.6)));
	size_t goldstone = players + static_cast<size_t>(round(pow(static_cast<double>(tiles), 0.23))) - 2;
	size_t bushes = players + static_cast<size_t>(round(pow(static_cast<double>(tiles), 0.25))) - 1;

	// TODO rewrite generation algorithm
	// FIXME ensure there are no static nor dynamic objects placed on top of each other

	printf("create %llu trees\n", (long long unsigned)trees);

	for (size_t i = 0; i < trees; ++i) {
		Box2<float> pos(
			static_cast<uint16_t>(lcg.next(10, map.w - 1)),
			static_cast<uint16_t>(lcg.next(map.h - 1))
		);
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::wood, (unsigned)DrsId::desert_tree + lcg.next() % 4));
	}

	printf("create %llu bushes\n", (long long unsigned)5 * bushes);

	for (size_t i = 0; i < bushes; ++i) {
		Box2<float> pos(
			static_cast<uint16_t>(lcg.next(12, map.w - 4 - 1)),
			static_cast<uint16_t>(lcg.next(4, map.h - 4 - 1))
		);
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::food, 240));
		pos.left += 1;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::food, 240));
		pos.top -= 1;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::food, 240));
		pos.left -= 1;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::food, 240));
		pos.left -= 1;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::food, 240));
	}

	printf("create %llu goldstone\n", (long long unsigned)4 * goldstone);

	for (size_t i = 0; i < goldstone; ++i) {
		Box2<float> pos(
			static_cast<uint16_t>(lcg.next(10, map.w - 10 - 1)),
			static_cast<uint16_t>(lcg.next(2, map.h - 2 - 1))
		);
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::gold, 481, lcg.next(6)));
		pos.left -= 2;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::gold, 481, lcg.next(6)));
		pos.left += 3;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::gold, 481, lcg.next(6)));
		pos.top += 1;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::gold, 481, lcg.next(6)));

		pos.left = static_cast<uint16_t>(lcg.next(10, map.w - 10 - 1));
		pos.top = static_cast<uint16_t>(lcg.next(2, map.h - 2 - 1));

		static_res.emplace_back(new StaticResource(map, pos, ResourceType::stone, 622, lcg.next(6)));
		pos.left -= 2;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::stone, 622, lcg.next(6)));
		pos.left += 3;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::stone, 622, lcg.next(6)));
		pos.top += 1;
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::stone, 622, lcg.next(6)));
	}

	printf("create %u players\n", players);

	for (unsigned i = 0; i < players; ++i) {
		Box2<float> pos(
			5, (static_cast<float>(i + 1) / (players + 1)) * map.h
		);
		buildings.emplace_back(new Building(map, pos, BuildingType::town_center, i));
		pos.top += 4;
		buildings.emplace_back(new Building(map, pos, BuildingType::barracks, i));
	}
}

static const DrsId build_anim_base[] = {
	DrsId::barracks_base,
	DrsId::town_center_base,
};

static const DrsId build_anim_player[] = {
	DrsId::empty3x3,
	DrsId::town_center_player,
};

static const unsigned build_hp[] = {
	350, // barracks
	600, // town center
};

Building::Building(Map &map, const Box2<float> &pos, BuildingType type, unsigned player)
	: Particle(map, pos, (unsigned)build_anim_base[(unsigned)type], 0, player)
	, Alive(build_hp[(unsigned)type])
	, anim_player((unsigned)build_anim_player[(unsigned)type]), player(player), prod() {}

void Building::tick(World &world) {
	auto &next = prod.front();
}

void World::query_static(std::vector<Particle*> &list, const Box2<float> &bounds) {
	for (auto &x : static_res)
		if (bounds.intersects(x->scr))
			list.push_back(x.get());

	for (auto &x : buildings)
		if (bounds.intersects(x->scr))
			list.push_back(x.get());
}

}

}
