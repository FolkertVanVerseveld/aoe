/* Copyright 2019 the Age of Empires Free Sofware Remake authors. See LEGAL for legal info */

#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#define DEFAULT_PORT 25659

#define IO_BUFSZ 4096
#define PATHSZ IO_BUFSZ

#define MOTD_SZ TEXT_BUFSZ
#define TEXT_BUFSZ (1024 - NET_HEADER_SIZE - 4)

#define INVALID_USER_ID UINT16_MAX

/** Maximum username length in UTF8 characters, note that this limit is 13 if each codepoint is encoded as 6 bytes. */
#define MAX_USERNAME 80
// this should be sufficient as long as there are no practical quantum computers yet
#define MAX_PASSWORD 80

#include <stdint.h>
#include <inttypes.h>

/** Maximum network packet size in bytes. This should be a power of two */
#define NET_PACKET_SIZE 1024
#define NET_HEADER_SIZE 4

#define NET_MESSAGE_SIZE (1024 - NET_HEADER_SIZE - 4)

#define NET_TEXT_RECP_ALL     INVALID_USER_ID
#define NET_TEXT_TYPE_USER    0
#define NET_TEXT_TYPE_SERVER  1
#define NET_TEXT_TYPE_ALLIES  2
#define NET_TEXT_TYPE_ENEMIES 3

#define NT_TEXT           0
#define NT_SERVER_CONTROL 1
#define NT_OP             2

#define NT_MAX 2

#define SC_STOP 0

#include <xt/endian.h>
#include <xt/utils.h>

#include <xt/socket.h>

#define tcp_read(sock, buf, bufsz, read) xtSocketTCPReadFully(sock, buf, bufsz, read, 3)
#define tcp_write(sock, buf, bufsz, sent) xtSocketTCPWriteFully(sock, buf, bufsz, sent, 3)

// TODO check and provide xtSocketTCPReadFully and xtSocketTCPWriteFully if lib is too old
int xtSocketTCPReadFully(xtSocket sock, void *restrict buf, uint16_t buflen, uint16_t *restrict bytesRead, unsigned retryCount);
int xtSocketTCPWriteFully(xtSocket sock, const void *restrict buf, uint16_t buflen, uint16_t *restrict bytesSent, unsigned retryCount);

struct net_text {
	uint16_t recipient, type;
	char text[TEXT_BUFSZ];
};

struct net_serverctl {
	uint16_t opcode, data;
};

struct net_op {
	char passwd[MAX_PASSWORD];
};

struct net_pkg {
	uint16_t type, length;
	union pdata {
		struct net_text text;
		struct net_serverctl serverctl;
		struct net_op op;
	} data;
};

void net_pkg_init2(struct net_pkg *p, unsigned type);
void net_pkg_init(struct net_pkg *p, unsigned type, ...);

/* Packet endian conversion. */
void net_pkg_ntoh(struct net_pkg *p);
void net_pkg_hton(struct net_pkg *p);

#endif
