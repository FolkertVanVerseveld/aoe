#include "os.hpp"

#include "base.hpp"
#include "os_macros.hpp"
#include "string.hpp"

#include <cassert>

#ifndef windows
#error stub
#endif

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

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