/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include <string>

namespace genie {

extern class OS final {
public:
	/**
	 * The UTF-8 representations for computer system name and current logged in username.
	 * Linux provides UTF-8 automagically, but on windows, it is converted from UTF16-LE.
	 */
	std::string compname, username;

	OS();
} os;

}
