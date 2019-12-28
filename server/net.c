/* Copyright 2019 the Age of Empires Free Sofware Remake authors. See LEGAL for legal info */

#include "common.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <xt/error.h>
#include <xt/string.h>

static const uint16_t nt_tblsz[] = {
	[NT_TEXT          ] = sizeof(struct net_text),
	[NT_SERVER_CONTROL] = sizeof(struct net_serverctl),
	[NT_OP            ] = sizeof(struct net_op),
};

#ifndef XT_SOCKET_TCP_IO_TRIES
#define XT_SOCKET_TCP_IO_TRIES 16
#endif

int xtSocketTCPReadFully(xtSocket sock, void *restrict buf, uint16_t buflen, uint16_t *restrict bytesRead, unsigned retryCount)
{
	int err;
	uint16_t size = 0, in = 0, rem = buflen;
	unsigned char *ptr = buf;

	if (!retryCount)
		retryCount = XT_SOCKET_TCP_IO_TRIES;

	for (unsigned i = 0; i < retryCount; ++i) {
		err = xtSocketTCPRead(sock, ptr, rem, &size);
		if (err)
			goto fail;

		in += size;
		if (in >= buflen)
			break;

		rem -= buflen;
		ptr += buflen;
	}

	if (!err && in < buflen)
		err = XT_EAGAIN;
fail:
	*bytesRead = in;
	return err;
}

int xtSocketTCPWriteFully(xtSocket sock, const void *restrict buf, uint16_t buflen, uint16_t *restrict bytesSent, unsigned retryCount)
{
	int err;
	uint16_t size = 0, out = 0, rem = buflen;
	const unsigned char *ptr = buf;

	if (!retryCount)
		retryCount = XT_SOCKET_TCP_IO_TRIES;

	for (unsigned i = 0; i < retryCount; ++i) {
		err = xtSocketTCPWrite(sock, ptr, rem, &size);
		if (err)
			goto fail;

		out += size;
		if (out >= buflen)
			break;

		rem -= out;
		ptr += out;
	}

	if (!err && out < buflen)
		err = XT_EAGAIN;
fail:
	*bytesSent = out;
	return err;
}

void net_pkg_init2(struct net_pkg *p, unsigned type)
{
	assert(type <= NT_MAX);

	p->type = type;
	p->length = nt_tblsz[type];

	memset(&p->data, 0, sizeof(union pdata));
}

static void net_pkg_init_text(struct net_pkg *p, va_list args)
{
	struct net_text *msg = &p->data.text;

	msg->recipient = va_arg(args, unsigned);
	msg->type = va_arg(args, unsigned);
	xtstrncpy(msg->text, va_arg(args, const char*), NET_MESSAGE_SIZE);
}

void net_pkg_init(struct net_pkg *p, unsigned type, ...)
{
	va_list args;
	va_start(args, type);

	net_pkg_init2(p, type);

	switch (type) {
	case NT_TEXT:
		net_pkg_init_text(p, args);
		break;
	case NT_SERVER_CONTROL:
		p->data.serverctl.opcode = va_arg(args, unsigned);
		p->data.serverctl.data = va_arg(args, unsigned);
		break;
	case NT_OP:
		xtstrncpy(p->data.op.passwd, va_arg(args, const char*), MAX_PASSWORD);
		break;
	default:
		assert(0);
		break;
	}

	va_end(args);
}

// Packet endian conversion

void net_pkg_ntoh(struct net_pkg *p)
{
	p->type = xtbe16toh(p->type);
	p->length = xtbe16toh(p->length);

	union pdata *data = &p->data;

	switch (p->type) {
	case NT_TEXT:
		data->text.recipient = xtbe16toh(data->text.recipient);
		data->text.type = xtbe16toh(data->text.type);
		break;
	case NT_SERVER_CONTROL:
		data->serverctl.opcode = xtbe16toh(data->serverctl.opcode);
		data->serverctl.data = xtbe16toh(data->serverctl.data);
		break;
	case NT_OP:
		break;
	default:
		assert(0);
		break;
	}
}

void net_pkg_hton(struct net_pkg *p)
{
	union pdata *data = &p->data;

	switch (p->type) {
	case NT_TEXT:
		data->text.recipient = xthtobe16(data->text.recipient);
		data->text.type = xthtobe16(data->text.type);
		break;
	case NT_SERVER_CONTROL:
		data->serverctl.opcode = xthtobe16(data->serverctl.opcode);
		data->serverctl.data = xthtobe16(data->serverctl.data);
		break;
	case NT_OP:
		break;
	default:
		assert(0);
		break;
	}

	p->type = xthtobe16(p->type);
	p->length = xthtobe16(p->length);
}
