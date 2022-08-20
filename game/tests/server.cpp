#include "../src/server.hpp"

#include <stdexcept>

namespace aoe {

static const char *default_host = "127.0.0.1";
static const uint16_t default_port = 1234;

static void server_create_fail() {
	try {
		Server s;
		(void)s;
		fprintf(stderr, "%s: server created with no network subsystem\n", __func__);
	} catch (std::runtime_error&) {}
}

static void client_create_fail() {
	try {
		Client c;
		(void)c;
		fprintf(stderr, "%s: client created with no network subsystem\n", __func__);
	} catch (std::runtime_error&) {}
}

static void server_create_delete() {
	Server s;
	(void)s;
}

static void server_create_twice() {
	Server s, s2;
	(void)s;
	(void)s2;
}

static void server_stop_early() {
	Server s;
	s.stop();
}

static void server_init_lazy() {
	Server s;
	s.start(default_port);
	s.stop();
}

static void server_create_port() {
	Server s(default_port);
	(void)s;
}

static void f(Server &s) {
	s.mainloop(-1);
}

static void client_init_delete() {
	Client c;
	(void)c;
}

static void client_stop_early() {
	Client c;
	c.stop();
}

static void client_runall() {
	puts("client");
	client_init_delete();
	client_stop_early();
}

static void connect_test(bool close) {
	Server s(default_port);
	std::thread t1([&] { s.mainloop(-1); if (close) s.close(); });

	Client c;
	c.start(default_host, default_port);

	// verify: client connected, server running
	if (!c.connected())
		fprintf(stderr, "%s: client should be connected to %s:%d\n", __func__, default_host, default_port);
	if (!s.running())
		fprintf(stderr, "%s: server should be running at %s:%d\n", __func__, default_host, default_port);

	c.stop();

	// verify: client disconnected, server thread still running
	if (c.connected())
		fprintf(stderr, "%s: client should be disconnected from %s:%d\n", __func__, default_host, default_port);

	t1.join();

	// verify: server should be running if close is false, stopped otherwise
	if (close == s.running())
		fprintf(stderr, "%s: server should %s at %s:%d\n", __func__, close ? "have closed" : "still be running", default_host, default_port);
}

static void connect_too_early() {
	Client c;

	try {
		c.start(default_host, default_port);
		fprintf(stderr, "%s: should not be able to connect to %s:%d\n", __func__, default_host, default_port);
	} catch (std::runtime_error&) {}
}

static void connect_too_late() {
	Server s(1234);
	s.close();

	try {
		Client c;
		c.start("127.0.0.1", default_port);
		fprintf(stderr, "%s: should not be able to connect to %s:%d\n", __func__, default_host, default_port);
	} catch (std::runtime_error&) {}
}

static void connect_runall() {
	puts("connect tests");
	connect_too_early();
	connect_too_late();
	connect_test(false);
	connect_test(true);
}

void server_runall() {
	puts("server");
	server_create_fail();
	client_create_fail();
	Net n;
	server_create_delete();
	server_create_twice();
	server_stop_early();
	server_init_lazy();
	server_create_port();

	client_runall();
	connect_runall();
}

}
