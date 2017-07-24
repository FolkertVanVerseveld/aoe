/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Age of Empires CD resource patcher
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static void print_error(const char *str)
{
	DWORD last_error;

	char buf[1024];

	last_error = GetLastError();

	if (last_error)
		FormatMessageA(0x1000, 0, last_error, 0x400, buf, sizeof buf, 0);
	else {
		printf("errno: %d\n", errno);
		snprintf(buf, sizeof buf, strerror(errno));
	}

	fprintf(stderr, "%s: %s\n", str, buf);
}

static int patch(HMODULE obj, HANDLE dest, LPCTSTR name, LPCTSTR type)
{
	HRSRC ptr;
	HGLOBAL res;
	LPVOID data;
	BOOL retval;

	ptr = FindResource(obj, name, type);

	if (!ptr) {
		print_error("Bad resource pointer");
		return 1;
	}

	res = LoadResource(obj, ptr);

	if (!res) {
		print_error("Bad resource");
		return 1;
	}

	data = LockResource(res);

	if (!data) {
		print_error("Could not lock resource");
		return 1;
	}

	retval = UpdateResource(
		dest,
		type,
		name,
		0x409, /* SUBLANG_ENGLISH_US */
		data,
		SizeofResource(obj, ptr)
	);

	if (!retval) {
		print_error("Could not patch resource");
		return 1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int retval = 1;

	HMODULE setup;
	HANDLE dest;

	if (argc != 3) {
		fprintf(
			stderr, "usage: %s source_setup destination_setup\n",
			argc > 0 ? argv[0] : "rp"
		);

		return 1;
	}

	setup = LoadLibrary(argv[1]);

	if (!setup) {
		print_error("Could not load source setup");
		return 1;
	}

	dest = BeginUpdateResource(argv[2], FALSE);

	if (!dest) {
		print_error("Could not load destination setup");
		goto free_setup;
	}

	patch(setup, dest, MAKEINTRESOURCE(0x65), "SETUPINFO");

	retval = 0;

free_setup:
	if (!FreeLibrary(setup)) {
		print_error("Could not free executable");

		if (!retval)
			retval = 1;
	}

	return retval;
}
