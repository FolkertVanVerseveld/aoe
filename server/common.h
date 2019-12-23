/* Copyright 2019 the Age of Empires Free Sofware Remake authors. See LEGAL for legal info */

#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#define DEFAULT_PORT 25659

#define IO_BUFSZ 4096
#define PATHSZ IO_BUFSZ

#define MOTD_SZ TEXT_BUFSZ
#define TEXT_BUFSZ 1024

#include <stdint.h>
#include <inttypes.h>

struct net_pkg {
	uint16_t type, length;
};

#endif
