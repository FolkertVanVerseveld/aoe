#include <cstdio>

#include "src/engine/ini.hpp"

#if _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#include "src/net/net.hpp"

using namespace aoe;

// TODO use config.ini

int main(int argc, char **argv) {
	IniParser ini("server.ini");
	if (!ini.exists())
		perror("server.ini");

	return 1;
}
#else
int main(int argc, char **argv) {
	fputs("dedicated server only supported on Windows...\n", stderr);
	return 1;
}
#endif
