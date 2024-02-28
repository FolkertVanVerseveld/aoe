#include "../src/engine.hpp"

#include <cstdio>

#include <stdexcept>
#include <thread>

#if _WIN32
#include <WinSock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#endif

#include <memory>
#include <ctime>

#include <gtest/gtest.h>

#include "util.hpp"

namespace aoe {

static const char *default_host = "127.0.0.1";
static const uint16_t default_port = 1234;

TEST(Net, StartStop) {
	Net net;
	(void)net;
}

TEST(Net, StartTwice) {
	Net net;
	try {
		// definitely not recommended to start the network subsystem twice, but it should work
		Net net2;
	} catch (std::exception &e) {
		FAIL() << e.what();
	} catch (...) {
		FAIL() << "unknown error";
	}
}

#if _WIN32
TEST(Net, Adapters) {
	DWORD ret = 0;

	// NOTE ptr is an array, but only a[0] is valid! to get a[1] use a[0]->Next
	std::unique_ptr<IP_ADAPTER_INFO[]> a(new IP_ADAPTER_INFO[1]);
	ULONG sz = sizeof(a[0]), err;

	struct tm newtime;
	char buffer[32];
	errno_t error;

	// assume first attempt will fail as we have no idea what size to expect, but it might succeed
	if ((err = GetAdaptersInfo(&a[0], &sz)) != 0 && err != ERROR_BUFFER_OVERFLOW)
		FAIL() << "GetAdaptersInfo: code " << err;

	// resize and retry if failed
	if (err) {
		a.reset(new IP_ADAPTER_INFO[sz / sizeof(a[0]) + 1]);

		if ((err = GetAdaptersInfo(&a[0], &sz)) != 0)
			FAIL() << "GetAdaptersInfo: code " << err;
	}

	puts("Network adapters:");

	bool has_eth = false;

	for (PIP_ADAPTER_INFO ai = &a[0]; ai; ai = ai->Next) {
		printf("\tComboIndex: \t%d\n", ai->ComboIndex);
		printf("\tAdapter Name: \t%s\n", ai->AdapterName);
		printf("\tAdapter Desc: \t%s\n", ai->Description);
		printf("\tAdapter Addr: \t");
		for (UINT i = 0; i < ai->AddressLength; i++) {
			if (i == (ai->AddressLength - 1))
				printf("%.2X\n", (int)ai->Address[i]);
			else
				printf("%.2X-", (int)ai->Address[i]);
		}

		printf("\tIndex: \t%d\n", ai->Index);
		printf("\tType: \t");
		switch (ai->Type) {
		case MIB_IF_TYPE_OTHER:
			puts("Other");
			break;
		case MIB_IF_TYPE_ETHERNET:
			puts("Ethernet");
			has_eth = true;
			break;
		case MIB_IF_TYPE_TOKENRING:
			puts("Token Ring");
			break;
		case MIB_IF_TYPE_FDDI:
			puts("FDDI");
			break;
		case MIB_IF_TYPE_PPP:
			puts("PPP");
			break;
		case MIB_IF_TYPE_LOOPBACK:
			puts("Lookback");
			break;
		case MIB_IF_TYPE_SLIP:
			puts("Slip");
			break;
		default:
			printf("Unknown type %ld\n", ai->Type);
			break;
		}

		printf("\tIP Address: \t%s\n", ai->IpAddressList.IpAddress.String);
		printf("\tIP Mask: \t%s\n", ai->IpAddressList.IpMask.String);
		printf("\tGateway: \t%s\n\t***\n", ai->GatewayList.IpAddress.String);

		if (ai->DhcpEnabled) {
			puts("\tDHCP Enabled: Yes");
			printf("\t  DHCP Server: \t%s\n", ai->DhcpServer.IpAddress.String);

			printf("\t  Lease Obtained: ");
			/* Display local time */
			error = _localtime32_s(&newtime, (__time32_t *)&ai->LeaseObtained);
			if (error) {
				ADD_FAILURE() << "Invalid Argument to _localtime32_s";
			} else {
				// Convert to an ASCII representation
				error = asctime_s(buffer, sizeof(buffer), &newtime);
				/* asctime_s returns the string terminated by \n\0 */
				if (error)
					ADD_FAILURE() << "Invalid Argument to asctime_s";
				else
					puts(buffer);
			}

			printf("\t  Lease Expires:  ");
			error = _localtime32_s(&newtime, (__time32_t *)&ai->LeaseExpires);
			if (error) {
				ADD_FAILURE() << "Invalid Argument to _localtime32_s";
			} else {
				// Convert to an ASCII representation
				error = asctime_s(buffer, sizeof(buffer), &newtime);
				/* asctime_s returns the string terminated by \n\0 */
				if (error)
					ADD_FAILURE() << "Invalid Argument to asctime_s";
				else
					puts(buffer);
			}
		} else {
			puts("\tDHCP Enabled: No");
		}

		if (ai->HaveWins) {
			puts("\tHave Wins: Yes");
			printf("\t  Primary Wins Server:    %s\n", ai->PrimaryWinsServer.IpAddress.String);
			printf("\t  Secondary Wins Server:  %s\n", ai->SecondaryWinsServer.IpAddress.String);
		} else {
			puts("\tHave Wins: No");
		}
		putchar('\n');
	}

	putchar('\n');

	if (!has_eth)
		ADD_FAILURE() << "no ethernet found";
}
#else
TEST(Net, Adapters) {
	GTEST_SKIP() << "only implemented on Windows at the moment";
}
#endif

class NoTracyFixture : public ::testing::Test {
protected:
	void SetUp() override {
#if TRACY_ENABLE
		GTEST_SKIP() << "skipping tcp init because tracy already has initialised network layer";
#endif
	}
};

TEST_F(NoUnixOrTracyFixture, TcpInitTooEarly) {
	try {
		TcpSocket tcp;
		FAIL() << "net should be initialised already";
	} catch (std::runtime_error&) {}
}

TEST_F(NoUnixOrTracyFixture, TcpInitTooLate) {
	try {
		{
			Net net;
		}
		TcpSocket tcp;
		FAIL() << "net should not be initialised anymore";
	} catch (std::runtime_error&) {}
}

TEST_F(NoTracyFixture, TcpInit) {
	Net net;
	TcpSocket tcp;
}

TEST(Tcp, BadAccept) {
	Net net;
	TcpSocket tcp;
	SOCKET s = tcp.accept();
	ASSERT_EQ(s, INVALID_SOCKET);
}

TEST(Tcp, BindDummy) {
	Net net;
	TcpSocket tcp(INVALID_SOCKET);
	try {
		tcp.bind("127.0.0.1", 4312);
		FAIL() << "invalid socket has been bound successfully";
	} catch (std::runtime_error&) {}
}

TEST(Tcp, TcpBindBadAddress) {
	Net net;
	TcpSocket tcp;
	try {
		tcp.bind("a.b.c.d", 1234);
		FAIL() << "should not bind to a.b.c.d:1234";
	} catch (std::runtime_error&) {}
}

TEST(Tcp, Bind) {
	Net net;
	TcpSocket tcp;
	tcp.bind("127.0.0.1", 4312);
}

TEST(Tcp, BindTwice) {
	Net net;
	TcpSocket tcp, tcp2;
	try {
		tcp.bind("127.0.0.1", 4312);
		tcp2.bind("127.0.0.1", 4312);
		FAIL() << "should not bind to 127.0.0.1:80";
	} catch (std::runtime_error&) {}
}

#if _WIN32
TEST(Tcp, ListenTooEarly) {
	Net net;
	TcpSocket tcp;
	try {
		tcp.listen(50);
		FAIL() << "should have called bind(address, port) first";
	} catch (std::runtime_error&) {}
}
#endif

TEST(Tcp, ListenBadBacklog) {
	Net net;
	TcpSocket tcp;
	tcp.bind("127.0.0.1", 4312);
	try {
		tcp.listen(-1);
		FAIL() << "backlog cannot be negative";
	} catch (std::runtime_error&) {}
	try {
		tcp.listen(0);
		FAIL() << "backlog must be positive";
	} catch (std::runtime_error&) {}
}

TEST(Tcp, ListenTwice) {
	Net net;
	TcpSocket tcp;
	tcp.bind("127.0.0.1", 4312);
	tcp.listen(50);
	tcp.listen(50);
}

TEST(Tcp, ConnectBadAddress) {
	Net net;
	TcpSocket tcp;
	try {
		tcp.connect("a.b.c.d", 4312);
		FAIL() << "should not recognize a.b.c.d";
	} catch (std::runtime_error&) {}
}

TEST(Tcp, ConnectTimeout) {
	Net net;
	TcpSocket tcp;
	try {
		tcp.connect(default_host, 4312);
		FAIL() << "should timeout";
	} catch (std::runtime_error&) {}
}

static void main_accept(std::vector<std::string> &bt) {
	try {
		TcpSocket server;
		server.bind(default_host, 4312);
		server.listen(1);
		SOCKET s = server.accept();
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

static void main_connect(std::vector<std::string> &bt) {
	try {
		TcpSocket client;
		client.connect(default_host, 4312);
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

static void main_receive_int(std::vector<std::string> &bt, int chk, bool equal) {
	try {
		TcpSocket server;

		server.bind(default_host, 4312);
		server.listen(1);

		SOCKET s = server.accept();
		TcpSocket peer(s);

		int v = chk ^ 0x5a5a5a5a, in = 0;
		try {
			in = peer.recv(&v, 1);
		} catch (std::runtime_error &e) {
			if (equal)
				bt.emplace_back(std::string("got ") + e.what());
		}

		char buf[64];
		buf[0] = '\0';

		if (equal) {
			if (in != 1)
				snprintf(buf, sizeof buf, "%s: requested %u bytes, but %d %s received\n", __func__, (unsigned)(sizeof v), in, in == 1 ? " byte" : "bytes");
			else if (v != chk)
				snprintf(buf, sizeof buf, "%s: expected 0x%X, got 0x%X (xor: 0x%X)\n", __func__, chk, v, chk ^ v);
		} else {
			if (in == 1)
				snprintf(buf, sizeof buf, "%s: requested %u bytes and %d %s received\n", __func__, (unsigned)(sizeof v), in, in == 1 ? " byte" : "bytes");
			else if (v == chk)
				snprintf(buf, sizeof buf, "did not expect %d", chk);
		}

		if (buf[0])
			bt.emplace_back(buf);
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

static void main_send_int(std::vector<std::string> &bt, int v) {
	try {
		TcpSocket client;

		client.connect(default_host, 4312);
		int out = client.send(&v, 1);

		char buf[64];

		if (out != 1) {
			snprintf(buf, sizeof buf, "%s: written %u bytes, but %d %s sent\n", __func__, (unsigned)v, out, out == 1 ? "byte" : "bytes");
			bt.emplace_back(buf);
		}
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

static void main_send_short(std::vector<std::string> &bt, short v) {
	try {
		TcpSocket client;

		client.connect("127.0.0.1", 4312);
		int out = client.send(&v, 1);

		char buf[64];

		if (out != 1) {
			snprintf(buf, sizeof buf, "%s: written %u bytes, but %d %s sent\n", __func__, (unsigned)v, out, out == 1 ? "byte" : "bytes");
			bt.emplace_back(buf);
		}
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static void main_exchange_receive(std::vector<std::string> &bt, unsigned step)
{
	try {
		TcpSocket server;

		server.bind(default_host, default_port);
		server.listen(1);

		SOCKET s = server.accept();
		TcpSocket peer(s);

		unsigned tbl[256]{ 0 };

		peer.recv_fully(tbl, ARRAY_SIZE(tbl));

		char buf[64];

		for (unsigned i = 0; i < ARRAY_SIZE(tbl); ++i) {
			if (tbl[i] != i * step) {
				snprintf(buf, sizeof buf, "%s: expected %u, got %u\n", __func__, i * step, tbl[i]);
				bt.emplace_back(buf);
				break;
			}
		}

		int dummy = 0;
		peer.send_fully(&dummy, 1);
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

static void main_exchange_send(std::vector<std::string> &bt, unsigned step)
{
	try {
		TcpSocket client;
		client.connect(default_host, default_port);

		unsigned tbl[256]{ 0 };

		for (unsigned i = 0; i < ARRAY_SIZE(tbl); ++i) {
			tbl[i] = i * step;
		}

		client.send_fully(&tbl[0], ARRAY_SIZE(tbl) / 2);
		client.send_fully(&tbl[ARRAY_SIZE(tbl) - ARRAY_SIZE(tbl) / 2], ARRAY_SIZE(tbl) / 2);

		int dummy;

		client.recv_fully(&dummy, 1);
	} catch (std::runtime_error &e) {
		bt.emplace_back(std::string("got ") + e.what());
	}
}

static void dump_errors(std::vector<std::string> &t1e, std::vector<std::string> &t2e) {
	if (!t1e.empty()) {
		for (size_t i = 0; i < t1e.size() - 1; ++i)
			ADD_FAILURE() << t1e[i];

		if (t2e.empty())
			FAIL() << t1e.back();
	}

	if (!t2e.empty()) {
		for (size_t i = 0; i < t2e.size() - 1; ++i)
			ADD_FAILURE() << t2e[i];

		FAIL() << t2e.back();
	}
}

// FIXME find out why it hangs on Github Actions
TEST_F(NoHeadlessFixture, TcpConnect) {
	Net net;
	std::vector<std::string> t1e, t2e;

	std::thread t1(main_accept, std::ref(t1e)), t2(main_connect, std::ref(t2e));
	t1.join();
	t2.join();

	dump_errors(t1e, t2e);
}

// FIXME find out why it hangs on Github Actions
TEST_F(NoHeadlessFixture, TcpExchange) {
	Net net;
	std::vector<std::string> t1e, t2e;

	std::thread t1(main_exchange_receive, std::ref(t1e), 5), t2(main_exchange_send, std::ref(t2e), 5);
	t1.join();
	t2.join();

	dump_errors(t1e, t2e);
}

// FIXME find out why it hangs on Github Actions
TEST_F(NoHeadlessFixture, TcpExchangeInt) {
	Net net;
	std::vector<std::string> t1e, t2e;
	int chk = 0xcafebabe;

	// assumes sockets on localhost always send and receive all data in a single call
	std::thread t1(main_receive_int, std::ref(t1e), chk, true), t2(main_send_int, std::ref(t2e), chk);

	t1.join();
	t2.join();

	dump_errors(t1e, t2e);
}

// FIXME find out why it hangs on Github Actions
TEST_F(NoHeadlessFixture, TcpSendFail) {
	Net net;
	std::vector<std::string> t1e, t2e;
	int chk = 0xcafebabe;

	std::thread t1(main_receive_int, std::ref(t1e), chk, false), t2(main_connect, std::ref(t2e));

	t1.join();
	t2.join();

	dump_errors(t1e, t2e);
}

// FIXME find out why it hangs on Github Actions
TEST_F(NoHeadlessFixture, TcpSendLessThanRecv) {
	Net net;
	std::vector<std::string> t1e, t2e;
	int chk = 0xcafebabe;

	std::thread t1(main_receive_int, std::ref(t1e), chk, false), t2(main_send_short, std::ref(t2e), chk);

	t1.join();
	t2.join();

	dump_errors(t1e, t2e);
}

TEST_F(NoUnixOrTracyFixture, SsockInitFail) {
	try {
		ServerSocket s;
		(void)s;
		FAIL() << "ssock created without network subsystem";
	} catch (std::runtime_error&) {}
}

TEST(Ssock, InitDelete) {
	Net net;
	ServerSocket s;
	(void)s;
}

TEST(Ssock, InitStop) {
	Net net;
	ServerSocket s;
	s.stop();
}

TEST(Ssock, OpenStop) {
	Net net;
	ServerSocket s;
	s.open(default_host, default_port);
	s.stop();
}

TEST(Ssock, ConnectStop) {
	Net net;
	ServerSocket s;
	s.open(default_host, default_port);
	std::thread t1([&] {
		TcpSocket dummy;
		dummy.connect(default_host, default_port);
		dummy.close();
	});
	SOCKET peer = s.accept();
	t1.join();
}

static bool process_echo(const Peer&, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed, void*) {
	for (; !in.empty(); in.pop_front())
		out.emplace_back(in.front());

	return true;
}

class SsockCtlDummy final : public ServerSocketController {
public:
	bool incoming(ServerSocket&, const Peer&) override { return true; }
	void dropped(ServerSocket&, const Peer&) override {}
	void stopped() override {}

	int proper_packet(ServerSocket&, const std::deque<uint8_t> &q) {
		return (int)(-(long long)q.size()); // always drop the data we receive. nom nom nom
	}

	bool process_packet(ServerSocket&, const Peer&, std::deque<uint8_t>&, std::deque<uint8_t> &out, int) { return true; }
};

class SsockCtlEcho final : public ServerSocketController {
public:
	bool incoming(ServerSocket&, const Peer&) override { return true; }
	void dropped(ServerSocket&, const Peer&) override {}
	void stopped() override {}

	int proper_packet(ServerSocket&, const std::deque<uint8_t> &q) {
		return !q.empty() ? 1 : 0; // always accept the data if the queue isn't empty
	}

	bool process_packet(ServerSocket&, const Peer&, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int) {
		for (; !in.empty(); in.pop_front())
			out.emplace_back(in.front());

		return true;
	}
};

class EpollGuard final {
public:
	~EpollGuard() {
#if _WIN32
		// hack to compensate on windows for epoll library
		WSACleanup();
#endif
	}
};

// FIXME find out why this segfaults on github actions (test linux locally as well!)
#if _WIN32
TEST(Ssock, mainloop) {
	EpollGuard g;
	Net net;
	std::vector<std::string> bt;

	std::thread t1([&] {
		SsockCtlDummy nomnom;
		ServerSocket s;
		int err = s.mainloop(default_port, 1, nomnom);
		if (err)
			bt.emplace_back("mainloop failed");
	});

	TcpSocket dummy;
	dummy.connect(default_host, default_port);
	dummy.close();
	t1.join();

	dump_errors(bt);
}

TEST(Ssock, mainloopSend) {
	EpollGuard g;
	Net net;
	std::vector<std::string> bt;

	std::thread t1([&] {
		SsockCtlDummy nomnom;
		ServerSocket s;
		int err = s.mainloop(default_port, 1, nomnom);
		if (err)
			bt.emplace_back("mainloop failed");
	});

	TcpSocket dummy;
	const char *msg = "Hello, k thx goodbye.";
	dummy.connect(default_host, default_port);
	dummy.send_fully(msg, (int)strlen(msg) + 1);
	dummy.close();
	t1.join();

	dump_errors(bt);
}

TEST(Ssock, mainloopEcho) {
	EpollGuard g;
	Net net;
	std::vector<std::string> bt;

	std::thread t1([&] {
		SsockCtlEcho echo;
		ServerSocket s;
		int err = s.mainloop(default_port, 1, echo);
		if (err)
			bt.emplace_back("mainloop failed");
	});

	TcpSocket dummy;
	char *msg = "Hello, k thx goodbye.";
	dummy.connect(default_host, default_port);
	dummy.send_fully(msg, (int)strlen(msg) + 1);

	std::vector<char> buf(strlen(msg) + 1, '\0');

	try { 
		dummy.recv_fully(buf.data(), (int)buf.size());

		if (memcmp(msg, buf.data(), buf.size()))
			ADD_FAILURE() << "bogus echo";
	} catch (const std::exception &e) {
		ADD_FAILURE() << "failed to receive reply: " << e.what();
	}

	dummy.close();
	t1.join();

	dump_errors(bt);
}
#endif

}
