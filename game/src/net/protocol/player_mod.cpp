#include "../../server.hpp"

#include <cassert>
#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::set_player_resize(size_t size) {
	if (size > UINT16_MAX)
		throw std::runtime_error("overflow player resize");

	PkgWriter out(*this, NetPkgType::playermod);
	write("2H", {(unsigned)NetPlayerControlType::resize, size}, false);
}

void NetPkg::claim_player_setting(uint16_t idx) {
	PkgWriter out(*this, NetPkgType::playermod);
	write("2H", {(unsigned)NetPlayerControlType::set_ref, idx}, false);
}

void NetPkg::set_player_died(uint16_t idx) {
	PkgWriter out(*this, NetPkgType::playermod);
	write("2H", {(unsigned)NetPlayerControlType::died, idx}, false);
}

void NetPkg::set_player_score(uint16_t idx, const PlayerAchievements &pa) {
	PkgWriter out(*this, NetPkgType::playermod);

	unsigned flags = 0;

	if (pa.alive) flags |= 1 << 0;

	write("2HILB",
		{
			(unsigned)NetPlayerControlType::set_score,
			idx,
			pa.military_score,
			pa.score,
			flags,
		}, false);
}

NetPlayerControl NetPkg::get_player_control() {
	ntoh();

	// TODO check data size
	if ((NetPkgType)hdr.type != NetPkgType::playermod)
		throw std::runtime_error("not a player control packet");

	std::vector<std::variant<uint64_t, std::string>> args;
	unsigned pos = 0;

	pos += read("H", args, pos);
	NetPlayerControlType type = (NetPlayerControlType)std::get<uint64_t>(args.at(0));

	switch (type) {
		case NetPlayerControlType::resize:
			pos += read("H", args, pos);
			return NetPlayerControl(type, (uint16_t)std::get<uint64_t>(args.at(1)));
		case NetPlayerControlType::erase:
		case NetPlayerControlType::died:
		case NetPlayerControlType::set_ref:
		case NetPlayerControlType::set_cpu_ref:
			pos += read("H", args, pos);
			return NetPlayerControl(type, (uint16_t)std::get<uint64_t>(args.at(1)));
		case NetPlayerControlType::set_player_name:
			pos += read("H40s", args, pos);
			return NetPlayerControl(type, (uint16_t)std::get<uint64_t>(args.at(1)), std::get<std::string>(args.at(2)));
		case NetPlayerControlType::set_civ:
		case NetPlayerControlType::set_team:
			pos += read("2H", args, pos);
			return NetPlayerControl(type, (uint16_t)std::get<uint64_t>(args.at(1)), (uint16_t)std::get<uint64_t>(args.at(2)));
		case NetPlayerControlType::set_score: {
			NetPlayerScore ps{ 0 };
			unsigned flags;

			pos += read("HILB", args, pos);
			ps.playerid = (uint16_t)std::get<uint64_t>(args.at(1));
			ps.military = (uint32_t)std::get<uint64_t>(args.at(2));
			ps.score = (int64_t)std::get<uint64_t>(args.at(3));

			flags = (uint8_t)std::get<uint64_t>(args.at(4));

			ps.alive = !!(flags & (1 << 0));

			return NetPlayerControl(ps);
		}
		default:
			throw std::runtime_error("bad player control type");
	}
}

void NetPkg::set_cpu_player(uint16_t idx) {
	PkgWriter out(*this, NetPkgType::playermod);
	write("2H", { (unsigned)NetPlayerControlType::set_cpu_ref, idx }, false);
}

void NetPkg::playermod2(NetPlayerControlType type, uint16_t idx, uint16_t pos) {
	PkgWriter out(*this, NetPkgType::playermod);
	write("3H", { (unsigned)type, idx, pos }, false);
}

void NetPkg::set_player_civ(uint16_t idx, uint16_t civ) {
	playermod2(NetPlayerControlType::set_civ, idx, civ);
}

void NetPkg::set_player_team(uint16_t idx, uint16_t team) {
	playermod2(NetPlayerControlType::set_team, idx, team);
}

void NetPkg::set_player_name(uint16_t idx, const std::string &s) {
	PkgWriter out(*this, NetPkgType::playermod);
	write("2H40s", { (unsigned)NetPlayerControlType::set_player_name, idx, s }, false);
}

}
