#pragma once

#if _WIN32
// use lowercase header names as we may be cross compiling on a filesystem where filenames are case-sensitive (which is typical on e.g. linux)
#include <winsock2.h>
#endif

namespace aoe {

class Net final {
public:
	Net();
	~Net();
};

}
