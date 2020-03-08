/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "world.hpp"

#include "net.hpp"
#include "random.hpp"

#include <cmath>

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

unsigned particle_id_counter = 0;

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
	100,
	400,
	250,
};

StaticResource::StaticResource(Map &map, const Box2<float> &pos, ResourceType type, unsigned res_anim, unsigned image)
	: Particle(map, pos, res_anim, image)
	, Resource(type, res_amount[(unsigned)type]) {}

World::World(LCG &lcg, const StartMatch &settings)
	: map(lcg, settings)
	, lcg(lcg)
	, static_res()
	//, tiled_objects(Vector2<int>(ispow2(settings.map_w) ? settings.map_w : nextpow2(settings.map_w), ispow2(settings.map_h) ? settings.map_h : nextpow2(settings.map_h)))
	//, movable_objects(Vector2<float>(static_cast<float>(settings.map_w), static_cast<float>(settings.map_h)))
{
}

void World::populate(const StartMatch &settings) {
	size_t tiles = static_cast<size_t>(settings.map_w) * settings.map_h;
	size_t trees = static_cast<size_t>(pow(static_cast<double>(tiles), 0.6));

	printf("create %llu trees\n", (long long unsigned)trees);

#if 1
	for (size_t i = 0; i < trees; ++i) {
		Box2<float> pos(
			static_cast<uint16_t>(lcg.next() % settings.map_w),
			static_cast<uint16_t>(lcg.next() % settings.map_h)
		);
		static_res.emplace_back(new StaticResource(map, pos, ResourceType::wood, 463));
	}

#else
	static_res.emplace_back(new StaticResource(map, Box2<float>(), ResourceType::wood, 463));
	static_res.emplace_back(new StaticResource(map, Box2<float>(settings.map_w - 1), ResourceType::wood, 463));
	static_res.emplace_back(new StaticResource(map, Box2<float>(0, settings.map_h - 1), ResourceType::wood, 463));
	static_res.emplace_back(new StaticResource(map, Box2<float>(settings.map_w - 1, settings.map_h - 1), ResourceType::wood, 463));
#endif
}

void World::query(std::vector<StaticResource*> &list, const Box2<float> &bounds) {
	for (auto &x : static_res)
		if (bounds.intersects(x->scr))
			list.push_back(x.get());
}

}

}
