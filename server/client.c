/* Copyright 2019 the Age of Empires Free Sofware Remake authors. See LEGAL for legal info */

/**
 * Age of Empires terminal client
 *
 * Provides a bare bones client to test various things.
 * NOTE this client does not support windows because it uses ncurses
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <stdint.h>
#include <inttypes.h>

#include <ncurses.h>

#include <xt/string.h>
#include <xt/socket.h>

#include "common.h"

xtSocket sockfd = XT_SOCKET_INVALID_FD;
static struct xtSockaddr sa;

char host[40] = "127.0.0.1";
uint16_t port = DEFAULT_PORT;

#define INIT_XTSOCKET 1
#define INIT_NCURSES 2

#define CONNECT_TRIES 3
#define CONNECT_TIMEOUT 1000

unsigned init = 0;

WINDOW *win = NULL;

int main(int argc, char **argv)
{
	int err = 1;

	if (argc != 3) {
		fprintf(stderr, "usage: %s [server_ip] [port]\n", argc > 0 ? argv[0] : "client");
		goto fail;
	}

	if (argc > 1) {
		if (argc == 3)
			port = atoi(argv[2]);

		xtstrncpy(host, argv[1], sizeof host);
	}

	if (!(xtSocketInit())) {
		fputs("main: internal error\n", stderr);
		goto fail;
	}

	init |= INIT_XTSOCKET;

	if ((err = xtSocketCreate(&sockfd, XT_SOCKET_PROTO_TCP))) {
		xtPerror("sock create", err);
		goto fail;
	}

	if ((err = xtSocketSetSoReuseAddress(sockfd, true))) {
		xtPerror("sock reuse", err);
		goto fail;
	}

	if ((err = xtSocketSetSoKeepAlive(sockfd, true))) {
		xtPerror("sock keep alive", err);
		goto fail;
	}

	if ((err = !xtSockaddrFromString(&sa, host, port))) {
		xtPerror("sockaddr init", err);
		goto fail;
	}

	for (unsigned i = 0; i < CONNECT_TRIES; ++i) {
		puts("connecting...");

		if (!(err = xtSocketConnect(sockfd, &sa)))
			goto connected;

		xtSleepMS(1000);
	}

	xtPerror("could not connect", err);
	err = 2;
	goto fail;

connected:
	puts("connected");

	if (!(win = initscr())) {
		perror("ncurses failed to start");
		goto fail;
	}

	init |= INIT_NCURSES;
	err = 0;
fail:
	if (init & INIT_NCURSES) {
		if (win)
			delscreen(win);
		endwin();
	}

	if (init & INIT_XTSOCKET) {
		if (sockfd != XT_SOCKET_INVALID_FD)
			xtSocketClose(&sockfd);
		xtSocketDestruct();
	}

	return err;
}
