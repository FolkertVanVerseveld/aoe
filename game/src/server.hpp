#pragma once

#include "net.hpp"

#include <mutex>
#include <deque>
#include <vector>

#if _WIN32
#include "../../wepoll/wepoll.h"
#endif

namespace aoe {

enum class NetPkgType {
	set_protocol,
};

struct NetPkgHdr final {
	uint16_t type, payload;
	bool native_ordering;

	static constexpr size_t size = 4;

	NetPkgHdr(uint16_t type, uint16_t payload, bool native=true) : type(type), payload(payload), native_ordering(native) {}

	// byte swapping
	void ntoh();
	void hton();
};

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;

	NetPkg() : hdr(0, 0, false), data() {}
	NetPkg(uint16_t type, uint16_t payload) : hdr(type, payload), data() {}
	/** munch as much data we need and check if valid. throws if invalid or not enough data. */
	NetPkg(std::deque<uint8_t> &q);

	void set_protocol(uint16_t version);
	uint16_t protocol_version();

	NetPkgType type();

	void ntoh();
	void hton();

	void write(std::deque<uint8_t> &q);
};

class ClientInfo final {

};

class Server final {
	ServerSocket s;
	std::atomic<bool> m_active;
	std::mutex m;
	uint16_t port, protocol;
	std::map<Peer, ClientInfo> peers;
public:
	Server();
	~Server();

	void stop();
	void close(); // this will block till everything is stopped

	bool active() const noexcept { return m_active; }

	int mainloop(int id, uint16_t port, uint16_t protocol);

	bool process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out);
private:
	bool chk_protocol(const Peer &p, std::deque<uint8_t> &out, uint16_t req);
};

class Client final {
	TcpSocket s;
	std::string host;
	uint16_t port;
	std::atomic<bool> m_connected;
	std::mutex m;
public:
	Client();
	~Client();

	void start(const char *host, uint16_t port);
	void stop();

	bool connected() const noexcept { return m_connected; }

	template<typename T> void send(T *ptr, int len=1) {
		s.send_fully(ptr, len);
	}

	template<typename T> void recv(T *ptr, int len=1) {
		s.recv_fully(ptr, len);
	}

	void send(NetPkg&);
	NetPkg recv();

	void send_protocol(uint16_t version);
	uint16_t recv_protocol();
};

}
