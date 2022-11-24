#include "../src/server.hpp"

#include <stdexcept>

#include "util.hpp"

namespace aoe {

static const char *default_host = "127.0.0.1";
static const uint16_t default_port = 1234;

static void server_create_fail() {
	try {
		Server s;
		(void)s;
		FAIL("server created with no network subsystem\n");
	} catch (std::runtime_error&) {}
}

static void client_create_fail() {
	try {
		Client c;
		(void)c;
		FAIL("client created with no network subsystem\n");
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

static void f(Server &s) {
	s.mainloop(1, default_port, 0);
}

static void client_init_delete() {
	Client c;
	(void)c;
}

static void client_stop_early() {
	Client c;
	c.stop();
}

static void client_tests() {
	puts("client");
	client_init_delete();
	client_stop_early();
}

static void connect_test(bool close) {
	Server s;
	std::thread t1([&] { s.mainloop(1, default_port, 0); if (close) s.close(); });

	Client c;
	c.start(default_host, default_port, false);

	// verify: client connected, server running
	if (!c.connected())
		FAIL("client should be connected to %s:%d\n", default_host, default_port);
	if (!s.active())
		FAIL("server should be activated at %s:%d\n", default_host, default_port);

	c.stop();

	// verify: client disconnected, server thread still running
	if (c.connected())
		FAIL("client should be disconnected from %s:%d\n", default_host, default_port);

	t1.join();

	// verify: server should be running if close is false, stopped otherwise
	if (close == s.active())
		FAIL("server should %s at %s:%d\n", close ? "have not be active" : "still be activated", default_host, default_port);
}

static void connect_too_early() {
	Client c;

	try {
		c.start(default_host, default_port, false);
		FAIL("should not be able to connect to %s:%d\n", default_host, default_port);
	} catch (std::runtime_error&) {}
}

static void connect_too_late() {
	Server s;
	s.close();

	try {
		Client c;
		c.start(default_host, default_port, false);
		FAIL("should not be able to connect to %s:%d\n", default_host, default_port);
	} catch (std::runtime_error&) {}
}

static void connect_runall() {
	puts("connect tests");
	connect_too_early();
	connect_too_late();
	connect_test(false);
	connect_test(true);
}

static void echo_test(bool close) {
	Server s;
	std::thread t1([&] { s.mainloop(1, default_port, 1); if (close) s.close(); });

	Client c;
	c.start(default_host, default_port, false);
	c.send_protocol(1);
	uint16_t prot = c.recv_protocol();

	if (prot != 1u)
		FAIL("bad protocol: expected %u, got %u\n", 1u, prot);

	c.stop();
	t1.join();
}

static void protocol_test() {
	Server s;
	std::thread t1([&] { s.mainloop(1, default_port, 1); s.close(); });

	Client c;
	uint16_t prot;

	c.start(default_host, default_port, false);
	// request newer version
	c.send_protocol(2);

	if ((prot = c.recv_protocol()) != 1u)
		FAIL("bad protocol: expected %u, got %u\n", 1u, prot);

	// request older version
	c.send_protocol(0);
	if ((prot = c.recv_protocol()) != 1u)
		FAIL("bad protocol: expected %u, got %u\n", 1u, prot);

	// request same version
	c.send_protocol(1);
	if ((prot = c.recv_protocol()) != 1u)
		FAIL("bad protocol: expected %u, got %u\n", 1u, prot);

	c.stop();
	t1.join();
}

static void data_exchange_runall() {
	puts("data exchange");
	echo_test(false);
	echo_test(true);
	protocol_test();
}

static void server_tests() {
	server_create_delete();
	server_create_twice();
	server_stop_early();
}

void server_runall() {
	puts("server");
#if TRACY_ENABLE
	fprintf(stderr, "%s: skipping server/client create fail because tracy already has initialised network layer\n", __func__);
#else
	server_create_fail();
	client_create_fail();
#endif
	Net n;
	server_tests();
	client_tests();
	connect_runall();
	data_exchange_runall();
}

}
