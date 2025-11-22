#include "src/engine.hpp"

#include <cstdio>

#include <memory>
#include <stdexcept>

#if _WIN32
#include <Windows.h>

static void show_error(const char *title, const char *message)
{
	fprintf(stderr, "%s: %s\n", title, message);
	MessageBox(NULL, message, title, MB_ICONERROR | MB_OK);
}
#else
#include <SDL2/SDL_messagebox.h>

static void show_error(const char *title, const char *message)
{
	fprintf(stderr, "%s: %s\n", title, message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, NULL);
}
#endif

int main(int argc, char **argv)
{
	try {
		std::unique_ptr<aoe::Engine> eng(new aoe::Engine());
		return eng->mainloop();
#if (defined(_DEBUG) && !_DEBUG) || (defined(NDEBUG) && !NDEBUG)
	} catch (const std::runtime_error &e) {
		show_error("Fatal Runtime Error", e.what());
	} catch (const std::exception &e) {
		show_error("Fatal Internal Error", e.what());
#endif
	} catch (int v) {
		// a la python: interpret as exit code
		// so we can do something 'clean' like: throw 0;
		return v;
	}
	return 1;
}
