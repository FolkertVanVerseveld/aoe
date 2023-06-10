#pragma once

#include <string>

#include <idpool.hpp>

namespace aoe {

enum class ClientInfoFlags {
	ready = 1 << 0,
};

class ClientInfo final {
public:
	std::string username;
	unsigned flags;
	IdPoolRef ref;

	ClientInfo() : username(), flags(0), ref(invalid_ref) {}
	ClientInfo(IdPoolRef ref, const std::string &username) : username(username), flags(0), ref(ref) {}
};

}
