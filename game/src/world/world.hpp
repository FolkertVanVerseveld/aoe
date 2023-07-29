#pragma once

#include <idpool.hpp>

#include "../net/protocol.hpp"

namespace aoe {

enum class WorldEventType {
	entity_add,
	entity_spawn,
	entity_kill,
	entity_task,
	player_kill,
	peer_cam_move,
	gameover,
	gamespeed_control,
};

class EventCameraMove final {
public:
	IdPoolRef ref;
	NetCamSet cam;

	EventCameraMove(IdPoolRef ref, const NetCamSet &cam) : ref(ref), cam(cam) {}
};

}
