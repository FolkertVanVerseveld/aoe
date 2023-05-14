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
	PkgWriter out(*this, NetPkgType::entity_mod);

	write("2H4IHHHBbbBHH", std::initializer_list<netarg>{
		(uint16_t)type, (uint16_t)e.type,
		e.ref.first, e.ref.second, e.x, e.y,
		e.angle * UINT16_MAX / (2 * M_PI),
		e.playerid, e.subimage,
		(uint8_t)e.state,
		(int8_t)(INT8_MAX * fmodf(e.x, 1)),
		(int8_t)(INT8_MAX * fmodf(e.y, 1)),
		e.stats.attack,
		e.stats.hp,
		e.stats.maxhp,
	}, false);
}

void NetPkg::set_entity_kill(IdPoolRef ref) {
	static_assert(sizeof(RefCounter) <= sizeof(uint32_t));
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::entity_mod);

	write("H2I", std::initializer_list<netarg> {
		(uint16_t)NetEntityControlType::kill,
		ref.first, ref.second,
	}, false);
}

void NetPkg::entity_move(IdPoolRef ref, float x, float y) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::entity_mod);

	write("2H4I", std::initializer_list<netarg>{
		(uint16_t)NetEntityControlType::task,
		(uint16_t)EntityTaskType::move,
		ref.first, ref.second, x, y
	}, false);
}

void NetPkg::entity_task(IdPoolRef r1, IdPoolRef r2, EntityTaskType type) {
	assert(type != EntityTaskType::move);
	refcheck(r1);
	refcheck(r2);
	PkgWriter out(*this, NetPkgType::entity_mod);

	write("2H4I", std::initializer_list<netarg>{
		(uint16_t)NetEntityControlType::task,
		(uint16_t)type,
		r1.first, r1.second,
		r2.first, r2.second,
	}, false);
}

void NetPkg::entity_train(IdPoolRef src, EntityType type) {
	ZoneScoped;
	refcheck(src);
	PkgWriter out(*this, NetPkgType::entity_mod);

	write("2H2IH", std::initializer_list<netarg> {
		(unsigned)NetEntityControlType::task,
		(uint16_t)EntityTaskType::train_unit,

		src.first, src.second,
		(unsigned)type, // TODO add more info to message when technologies are supported
	}, false);
}

NetEntityMod NetPkg::get_entity_mod() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::entity_mod)
		throw std::runtime_error("not an entity control packet");

	args.clear();
	unsigned pos = read("H", args);
	NetEntityControlType type = (NetEntityControlType)u16(0);

	switch (type) {
	case NetEntityControlType::add:
	case NetEntityControlType::spawn:
	case NetEntityControlType::update: {
		EntityView ev;
		args.clear();

		pos += read("H4IHHHBbbBHH", args, pos);

		ev.type = (EntityType)u16(0);
		ev.ref.first = u32(1); ev.ref.second = u32(2);
		ev.x = u32(3); ev.y = u32(4);

		ev.angle = u16(5) * (2 * M_PI) / UINT16_MAX;
		ev.playerid = u16(6);
		ev.subimage = u16(7);

		ev.state = (EntityState)u8(8);
		int8_t dx = i8(9), dy = i8(10);

		if (dx || dy) {
			ev.x += dx / (float)INT8_MAX;
			ev.y += dy / (float)INT8_MAX;
		}

		ev.stats.attack = u8(11);
		ev.stats.hp     = u16(12);
		ev.stats.maxhp  = u16(13);

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
		args.clear();
		pos += read("H", args, pos);
		EntityTaskType type = (EntityTaskType)u16(0);

		switch (type) {
			case EntityTaskType::move: {
				pos += read("4I", args, pos);

				return NetEntityMod(EntityTask(IdPoolRef(u32(1), u32(2)), u32(3), u32(4)));
			}
			case EntityTaskType::attack:
			case EntityTaskType::infer: {
				pos += read("4I", args, pos);

				return NetEntityMod(EntityTask(type, IdPoolRef(u32(1), u32(2)), IdPoolRef(u32(3), u32(4))));
			}
			case EntityTaskType::train_unit: {
				pos += read("2IH", args, pos);

				IdPoolRef src;
				src.first  = u32(1);
				src.second = u32(2);

				EntityType train = (EntityType)u16(3);

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