#include "../../server.hpp"

#include <cassert>
#include <except.hpp>

namespace aoe {

static void refcheck(const IdPoolRef &ref) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");
}

void NetPkg::set_entity_add(const Entity &e) {
	EntityView ev(e);
	set_entity_add(ev);
}

void NetPkg::set_entity_add(const EntityView &e) {
	entity_add(e, NetEntityControlType::add);
}

void NetPkg::set_entity_spawn(const Entity &e) {
	EntityView ev(e);
	set_entity_spawn(ev);
}

void NetPkg::set_entity_spawn(const EntityView &e) {
	entity_add(e, NetEntityControlType::spawn);
}

void NetPkg::set_entity_update(const Entity &e) {
	entity_add(EntityView(e), NetEntityControlType::update);
}

void NetPkg::entity_add(const EntityView &e, NetEntityControlType type) {
	static_assert(sizeof(float) <= sizeof(uint32_t));

	refcheck(e.ref);

	data.resize(NetEntityMod::addsize);

	/*
	layout:
	b # sz  name
	2 1 u16 type
	2 1 u16 e.type
	8 2 u32 e.ref
	4 1 u32 e.x
	4 1 u32 e.y
	2 1 u16 e.angle
	2 1 u16 e.color
	2 1 u16 e.subimage
	1 1 u8  e.state
	1 1 s8  e.dx
	1 1 s8  e.dy
	1 1 u8  e.stats.attack
	2 1 u16 e.stats.hp
	2 1 u16 e.stats.maxhp
	*/

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)type;
	dw[1] = (uint16_t)e.type;

	uint32_t *dd = (uint32_t*)&dw[2];

	dd[0] = e.ref.first;
	dd[1] = e.ref.second;

	dd[2] = (uint32_t)e.x;
	dd[3] = (uint32_t)e.y;

	dw = (uint16_t*)&dd[4];
	int16_t *sw = (int16_t*)dw;
	dw[0] = e.angle * UINT16_MAX / (2 * M_PI);
	dw[1] = e.playerid;
	dw[2] = e.subimage;

	uint8_t *db = (uint8_t*)&dw[3];
	int8_t *sb = (int8_t*)db;

	db[0] = (uint8_t)e.state;
	sb[1] = (int8_t)(INT8_MAX * fmodf(e.x, 1));
	sb[2] = (int8_t)(INT8_MAX * fmodf(e.y, 1));
	assert(e.stats.attack <= UINT8_MAX);
	db[3] = e.stats.attack;

	dw[5] = e.stats.hp;
	dw[6] = e.stats.maxhp;

	set_hdr(NetPkgType::entity_mod);
}

void NetPkg::set_entity_kill(IdPoolRef ref) {
	static_assert(sizeof(RefCounter) <= sizeof(uint32_t));

	refcheck(ref);

	PkgWriter out(*this, NetPkgType::entity_mod);

	write("H2I", std::initializer_list<netarg> {
		htons((uint16_t)NetEntityControlType::kill),
		ref.first, ref.second,
	}, false);
}

void NetPkg::entity_move(IdPoolRef ref, float x, float y) {
	refcheck(ref);

	PkgWriter out(*this, NetPkgType::entity_mod, NetEntityMod::tasksize);

	uint16_t *dw = (uint16_t*)data.data();
	uint32_t *dd;

	dw[0] = (uint16_t)NetEntityControlType::task;
	dw[1] = (uint16_t)EntityTaskType::move;

	dd = (uint32_t*)&dw[2];

	dd[0] = ref.first; dd[1] = ref.second;
	dd[2] = (uint32_t)x; dd[3] = (uint32_t)y;
}

void NetPkg::entity_task(IdPoolRef r1, IdPoolRef r2, EntityTaskType type) {
	assert(type != EntityTaskType::move);
	refcheck(r1);
	refcheck(r2);

	PkgWriter out(*this, NetPkgType::entity_mod, NetEntityMod::tasksize);

	uint16_t *dw = (uint16_t*)data.data();
	uint32_t *dd;

	dw[0] = (uint16_t)NetEntityControlType::task;
	dw[1] = (uint16_t)type;

	dd = (uint32_t*)&dw[2];

	dd[0] = r1.first; dd[1] = r1.second;
	dd[2] = r2.first; dd[3] = r2.second;
}

void NetPkg::entity_train(IdPoolRef src, EntityType type) {
	ZoneScoped;
	refcheck(src);

	PkgWriter out(*this, NetPkgType::entity_mod);

	write("2H2IH",
		std::initializer_list<std::variant<uint64_t, std::string>>
		{
			// TODO remove htons hack when all entity mod messages are converted
			htons((unsigned)NetEntityControlType::task),
			htons((unsigned)EntityTaskType::train_unit),

			src.first, src.second,
			(unsigned)type, // TODO add more info to message when technologies are supported
		}, false);
}

NetEntityMod NetPkg::get_entity_mod() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::entity_mod)
		throw std::runtime_error("not an entity control packet");

	const uint16_t *dw = (const uint16_t*)data.data();
	const int16_t *sw;
	const uint32_t *dd;
	const uint8_t *db;
	const int8_t *sb;

	NetEntityControlType type = (NetEntityControlType)dw[0];
	unsigned pos = 0;

	// TODO convert message to read/write netrw stuff
	pos += 2;

	switch (type) {
	case NetEntityControlType::add:
	case NetEntityControlType::spawn:
	case NetEntityControlType::update: {
		EntityView ev;

		ev.type = (EntityType)dw[1];

		dd = (const uint32_t*)&dw[2];
		ev.ref.first = dd[0]; ev.ref.second = dd[1];
		ev.x = dd[2]; ev.y = dd[3];

		dw = (const uint16_t*)&dd[4];
		sw = (const int16_t*)dw;
		ev.angle = dw[0] * (2 * M_PI) / UINT16_MAX;
		ev.playerid = dw[1];
		ev.subimage = dw[2];

		db = (const uint8_t*)&dw[3];
		sb = (const int8_t*)db;
		ev.state = (EntityState)db[0];
		if (sb[1] || sb[2]) {
			ev.x += sb[1] / (float)INT8_MAX;
			ev.y += sb[2] / (float)INT8_MAX;
		}

		ev.stats.attack = db[3];

		ev.stats.hp = dw[5];
		ev.stats.maxhp = dw[6];

		return NetEntityMod(ev, type);
	}
	case NetEntityControlType::kill: {
		IdPoolRef ref;
		args.clear();

		pos += read("2I", args, pos);
		ref.first  = u32(0);
		ref.second = u32(1);

		return NetEntityMod(ref);
	}
	case NetEntityControlType::task: {
		IdPoolRef ref1;

		EntityTaskType type = (EntityTaskType)dw[1];
		pos += 2;
		dd = (uint32_t*)&dw[2];

		ref1.first = dd[0]; ref1.second = dd[1];

		switch (type) {
			case EntityTaskType::move: {
				uint32_t x, y;
				x = dd[2]; y = dd[3];
				return NetEntityMod(EntityTask(ref1, x, y));
			}
			case EntityTaskType::attack:
			case EntityTaskType::infer: {
				IdPoolRef ref2;
				ref2.first = dd[2]; ref2.second = dd[3];
				return NetEntityMod(EntityTask(type, ref1, ref2));
			}
			case EntityTaskType::train_unit: {
				std::vector<std::variant<uint64_t, std::string>> args;
				pos += read("2IH", args, pos);

				IdPoolRef src;
				src.first = (uint32_t)std::get<uint64_t>(args.at(0));
				src.second = (uint32_t)std::get<uint64_t>(args.at(1));

				EntityType train = (EntityType)(uint16_t)std::get<uint64_t>(args.at(2));

				return NetEntityMod(EntityTask(src, train));
			}
			default:
				break;
		}

		throw std::runtime_error("unknown entity task type");
	}
	default:
		throw std::runtime_error("unknown entity control packet");
	}
}


}