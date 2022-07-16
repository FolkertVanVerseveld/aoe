#include "../src/sdl.hpp"
#include "../src/time.h"

#include <cstddef>
#include <cstring>

#include <memory>
#include <stdexcept>

namespace aoe {

static bool skip_chk_stall;

class SDL_guard final {
public:
	Uint32 flags;

	SDL_guard(Uint32 flags=SDL_INIT_VIDEO) : flags(flags) {
		if (SDL_Init(flags))
			throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
	}

	~SDL_guard() {
		SDL_Quit();
	}
};

static bool check_error(const char *func, size_t lno) {
	bool fail = false;

	const char *err = SDL_GetError();
	if (err && err[strspn(err, " \t")]) {
		fprintf(stderr, "%s:%llu: %s\n", func, (unsigned long long)lno, err);
		fail = true;
	}

	SDL_ClearError();
	return fail;
}

static bool is_error() {
	const char *err = SDL_GetError();
	return err && err[strspn(err, " \t")];
}

#define chkerr() check_error(__func__, __LINE__)

/**
 * Check if SDL can do error checking.
 * If it fails, all tests will be unreliable!
 */
static bool sdl_error() {
	const char *exp = "test", *got;
	bool fail = false;

	SDL_SetError(exp);
	if (strcmp(got = SDL_GetError(), exp)) {
		fprintf(stderr, "%s: expected \"%s\", but got \"%s\"\n", __func__, exp, got);
		fail = true;
	}
	SDL_ClearError();
	if (strcmp(got = SDL_GetError(), exp = "")) {
		fprintf(stderr, "%s: expected \"%s\", but got \"%s\"\n", __func__, exp, got);
		fail = true;
	}

	return fail;
}

static void quit_before_init() {
	SDL_Quit();
	chkerr();
}

static void init() {
	SDL_Init(0);
	SDL_GetNumVideoDisplays();
	if (!is_error())
		fprintf(stderr, "%s: main not ready, but init works\n", __func__);
	SDL_Quit();
}

static bool init_check_stall() {
	timespec before, after;

	clock_gettime(CLOCK_MONOTONIC, &before);
	SDL_Init(SDL_INIT_VIDEO);
	chkerr();
	SDL_Quit();
	clock_gettime(CLOCK_MONOTONIC, &after);

	double dt_ref = elapsed_time(&before, &after);

	clock_gettime(CLOCK_MONOTONIC, &before);
	SDL_Init(SDL_INIT_EVERYTHING);
	chkerr();
	SDL_Quit();
	clock_gettime(CLOCK_MONOTONIC, &after);

	double dt = elapsed_time(&before, &after);
	double slowdown = dt / dt_ref;

	if (dt > 10 && slowdown > 3) {
		fprintf(stderr, "%s: dt_ref=%.2f sec, dt=%.2f sec (slowdown of %.2f)\n", __func__, dt_ref, dt, slowdown);
		fprintf(stderr, "%s: compiled for SDL %d.%d.%d\n", __func__, SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
		return true;
	}

	return false;
}

static void query_displays() {
	SDL_guard sdl;

	int count = SDL_GetNumVideoDisplays();
	chkerr();
	if (count < 1)
		fprintf(stderr, "%s: expected positive count, but got %d\n", __func__, count);
}

void sdl_runall() {
	if (sdl_error()) {
		fprintf(stderr, "%s: cannot do error checking. skipping!\n", __func__);
		return;
	}

	quit_before_init();
	init();

	if (!skip_chk_stall)
		init_check_stall();
	else
		fprintf(stderr, "%s: skip init_check_stall\n", __func__);

	query_displays();
}

}
