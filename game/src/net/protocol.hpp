#pragma once

#include <cstddef>
#include <cstdint>

namespace aoe {

enum class NetPkgType {
	set_protocol,
	chat_text,
	start_game,
	set_scn_vars,
	set_username,
	playermod,
	peermod,
	terrainmod,
	resmod,
	entity_mod,
	gameover,
	cam_set,
	gameticks,
};

struct NetPkgHdr final {
	uint16_t type, payload;
	bool native_ordering;

	static constexpr size_t size = 4;

	NetPkgHdr(uint16_t type, uint16_t payload, bool native=true) : type(type), payload(payload), native_ordering(native) {}

	// byte swapping
	void ntoh();
	void hton();
};

class NetCamSet final {
public:
	int32_t x, y, w, h;

	static constexpr size_t size = 4 * sizeof(int32_t);

	NetCamSet() : x(0), y(0), w(1), h(1) {}
	NetCamSet(int32_t x, int32_t y, int32_t w, int32_t h) : x(x), y(y), w(w), h(h) {}
};

}
