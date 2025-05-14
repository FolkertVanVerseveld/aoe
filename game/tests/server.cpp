#include "../src/server.hpp"

#include <stdexcept>

#include <gtest/gtest.h>

#include "util.hpp"

namespace aoe {

static const char *default_host = "127.0.0.1";
static const uint16_t default_port = 1234;

class ServerFixture : public ::testing::Test {
	Net net;
};

TEST_F(NoUnixOrTracyFixture, serverCreateFail) {
	try {
		Server s;
		(void)s;
		FAIL() << "server created with no network subsystem";
	} catch (std::runtime_error&) {}
}

TEST_F(NoUnixOrTracyFixture, clientCreateFail) {
	try {
		Client c;
		(void)c;
		FAIL() << "client created with no network subsystem";
	} catch (std::runtime_error&) {}
}

TEST_F(ServerFixture, createDelete) {
	Server s;
	(void)s;
}

TEST_F(ServerFixture, createTwice) {
	Server s, s2;
	(void)s;
	(void)s2;
}

TEST_F(ServerFixture, serverStopEarly) {
	Server s;
	s.stop();
}

static void f(Server &s) {
	s.mainloop(1, default_port, 0);
}

TEST_F(ServerFixture, initDelete) {
	Client c;
	(void)c;
}

TEST_F(ServerFixture, clientStopEarly) {
	Client c;
	c.stop();
}

static void connect_test(bool close) {
	Server s;
	std::thread t1([&] { s.mainloop(1, default_port, 0, true); if (close) s.close(); });

	Client c;
	c.start(default_host, default_port, false);

	char buf[64];
	buf[0] = '\0';

	// verify: client connected, server running
	if (!c.connected())
		snprintf(buf, sizeof buf, "client should be connected to %s:%d", default_host, default_port);
	if (!s.active())
		snprintf(buf, sizeof buf, "server should be activated at %s:%d\n", default_host, default_port);

	if (buf[0])
		FAIL() << buf;

	c.stop();

	// verify: client disconnected, server thread still running
	if (c.connected())
		snprintf(buf, sizeof buf, "client should be disconnected from %s:%d\n", default_host, default_port);

	t1.join();

	// verify: server should be running if close is false, stopped otherwise
	if (close == s.active())
		snprintf(buf, sizeof buf, "server should %s at %s:%d\n", close ? "have not be active" : "still be activated", default_host, default_port);

	if (buf[0])
		FAIL() << buf;
}

TEST_F(ServerFixture, connectTests) {
#if BUILD_TESTS_HEADLESS
	GTEST_SKIP() << "todo figure out why this segfaults when running headless on github actions";
#else
	connect_test(false);
	connect_test(true);
#endif
}

TEST_F(ServerFixture, connectTooEarly) {
	Client c;

	try {
		c.start(default_host, default_port, false);

		char buf[64];
		snprintf(buf, sizeof buf, "should not be able to connect to %s:%d\n", default_host, default_port);
		FAIL() << buf;
	} catch (std::runtime_error&) {}
}

TEST_F(ServerFixture, connectTooLate) {
	Server s;
	s.close();

	try {
		Client c;
		c.start(default_host, default_port, false);

		char buf[64];
		snprintf(buf, sizeof buf, "should not be able to connect to %s:%d\n", default_host, default_port);
		FAIL() << buf;
	} catch (std::runtime_error&) {}
}

static void handshake(Client &c) {
	// ignore messages
	NetPkg pkg = c.recv();
	pkg.get_peer_control();

	pkg = c.recv();
	pkg.chat_text();

	pkg = c.recv();
	pkg.get_peer_control();

	pkg = c.recv();
	pkg.username();

	pkg = c.recv();
	pkg.get_player_control();
}

static void echo_test(std::vector<std::string> &bt, bool close) {
#if BUILD_TESTS_HEADLESS
	GTEST_SKIP() << "todo figure out why this segfaults when running headless on github actions";
#endif
	Net net;
	Server s;
	std::thread t1([&] { s.mainloop(default_port, 1, true); if (close) s.close(); });

	Client c;
	c.start(default_host, default_port, false);

	handshake(c);

	c.send_protocol(1);
	uint16_t prot = c.recv_protocol();

	if (prot != 1u) {
		char buf[64];
		snprintf(buf, sizeof buf, "bad protocol: expected %u, got %u\n", 1u, prot);
		bt.emplace_back(buf);
	}

	c.stop();
	t1.join();
}

TEST_F(ServerFixture, protocol) {
#if BUILD_TESTS_HEADLESS
	GTEST_SKIP() << "todo figure out why this segfaults when running headless on github actions";
#endif
	std::vector<std::string> bt;
	Server s;
	std::thread t1([&] { s.mainloop(default_port, 1, true); s.close(); });

	Client c;
	uint16_t prot;

	c.start(default_host, default_port, false);

	handshake(c);

	// request newer version
	c.send_protocol(2);

	char buf[64];

	if ((prot = c.recv_protocol()) != 1u) {
		snprintf(buf, sizeof buf, "bad protocol: expected %u, got %u", 1u, prot);
		bt.emplace_back(buf);
	}

	// request older version
	c.send_protocol(0);
	if ((prot = c.recv_protocol()) != 1u) {
		snprintf(buf, sizeof buf, "bad protocol: expected %u, got %u", 1u, prot);
		bt.emplace_back(buf);
	}

	// request same version
	c.send_protocol(1);
	if ((prot = c.recv_protocol()) != 1u) {
		snprintf(buf, sizeof buf, "bad protocol: expected %u, got %u", 1u, prot);
		bt.emplace_back(buf);
	}

	c.stop();
	t1.join();

	dump_errors(bt);
}

TEST_F(ServerFixture, dataExchange) {
#if BUILD_TESTS_HEADLESS
	GTEST_SKIP() << "todo figure out why this segfaults when running headless on github actions";
#endif
	std::vector<std::string> bt;

	echo_test(bt, false);
	echo_test(bt, true);

	dump_errors(bt);
}

}
