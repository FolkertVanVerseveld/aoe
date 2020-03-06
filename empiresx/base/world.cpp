/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "world.hpp"

#include "net.hpp"

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

StaticResource::StaticResource(const Box2<int> &pos, ResourceType type, unsigned res)
	: Unit(res_hp[(unsigned)type], pos, res ? res : res_anim[(unsigned)type], 0)
	, Resource(type, res_amount[(unsigned)type]) {}

World::World(LCG &lcg, const StartMatch &settings)
	: map(lcg, settings)
	, tiled_objects(Vector2<int>(ispow2(settings.map_w) ? settings.map_w : nextpow2(settings.map_w), ispow2(settings.map_h) ? settings.map_h : nextpow2(settings.map_h)))
	, movable_objects(Vector2<float>(static_cast<float>(settings.map_w), static_cast<float>(settings.map_h))) {}

}

}
