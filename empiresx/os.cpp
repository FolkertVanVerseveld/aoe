/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "os.hpp"

#include "base.hpp"
#include "os_macros.hpp"
#include "string.hpp"

#include <cassert>

#if windows
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#elif linux
#include <cstdlib>

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include <memory>
#endif

namespace genie {

OS os;

#if windows
OS::OS() : compname("DinnurBlaster"), username("King Harkinian") {
	TCHAR buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD count = ARRAY_SIZE(buf);
	BOOL b;

	b = GetComputerName(buf, &count);
	assert(b);

	this->compname = std::string(buf, count - 1);

	count = ARRAY_SIZE(buf);

	if (GetUserName(buf, &count)) {
		this->username = std::string(buf, count - 1);
	} else {
		TCHAR *buf2 = new TCHAR[count];

		b = GetUserName(buf2, &count);
		assert(b);

		this->username = std::string(buf2, count - 1);

		delete[] buf2;
	}
}

#elif linux

OS::OS() : compname("DinnurBlaster"), username(getpwuid(getuid())->pw_name) {
	char *buf = NULL;
	size_t count = 0;

	errno = 0;

	while (gethostname(buf, count)) {
		if ((errno != EINVAL && errno != ENAMETOOLONG) || count >= SIZE_MAX / 2)
			break;

		count = count ? count << 1 : 256;

		if (buf)
			delete[] buf;
		buf = new char[count];
	}

	buf[count - 1] = '\0';
	this->compname = buf;
	delete[] buf;
}
#else
#error stub
#endif

}
