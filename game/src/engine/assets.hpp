#pragma once

#include <string>

namespace aoe {

class Engine;

class Assets final {
public:
	Assets(int id, Engine &e, const std::string &path);
};

}
