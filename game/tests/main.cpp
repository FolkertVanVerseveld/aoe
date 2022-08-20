#define SDL_MAIN_HANDLED 1

#include "../src/engine.hpp"

namespace aoe {

extern void sdl_runall();
extern void net_runall();
extern void server_runall();

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

static void runall() {
	net_runall();
	server_runall();
	sdl_runall();
	engine_runall();
}

}

int main(void)
{
	aoe::runall();
	return 0;
}
