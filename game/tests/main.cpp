#define SDL_MAIN_HANDLED 1

#include "../src/engine.hpp"

namespace aoe {

extern void sdl_runall();

static void engine_create_delete() {
	Engine eng;
	(void)eng;
}

static void engine_create_twice() {
	Engine eng1;
	try {
		Engine eng2;
		fprintf(stderr, "%s: created twice\n", __func__);
	} catch (std::runtime_error &e) {}
}

static void engine_runall() {
	engine_create_delete();
	engine_create_twice();
}

static void net_create_delete() {
	Net net;
	(void)net;
}

static void net_create_twice() {
	Net net, net2;
	(void)net;
	(void)net2;
}

static void net_runall() {
	net_create_delete();
	net_create_twice();
}

static void runall() {
	net_runall();
	sdl_runall();
	engine_runall();
}

}

int main(void)
{
	aoe::runall();
	return 0;
}
