#include "src/engine.hpp"

#include <cstdio>

#include <memory>
#include <stdexcept>

int main(int argc, char **argv)
{
	try {
		std::unique_ptr<aoe::Engine> eng(new aoe::Engine());
		return eng->mainloop();
	} catch (const std::runtime_error &e) {
		fprintf(stderr, "runtime error: %s\n", e.what());
	} catch (const std::exception &e) {
		fprintf(stderr, "internal error: %s\n", e.what());
	} catch (int v) {
		return v;
	}
	return 1;
}
