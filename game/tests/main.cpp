#include "../src/engine.hpp"

namespace aoe {

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

static void runall() {
	engine_create_delete();
	engine_create_twice();
}

}

int main(void)
{
	aoe::runall();
	return 0;
}