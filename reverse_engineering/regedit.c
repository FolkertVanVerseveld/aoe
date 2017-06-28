/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Simple registry inspector that looks up and prints all registry keys.
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 *
 * It is completely written from scratch without consulting the dissassembly.
 */
#include <windows.h>
#include <stdio.h>

#define REG_PATH "Software\\Microsoft\\Microsoft Games\\Age of Empires\\2.0"
#define REG_XPATH "Software\\Microsoft\\Microsoft Games\\Age of Empires Expansion\\1.0"

struct option {
	const char *name;
	unsigned type;
};

static const struct option attr[] = {
	{"Advanced Buttons", REG_DWORD},
	{"Default Age of Empires Multiplayer Service", REG_SZ},
	{"Difficulty", REG_DWORD},
	{"Game Speed", REG_DWORD},
	{"Graphics Detail Level", REG_DWORD},
	{"Mouse Style", REG_DWORD},
	{"MP Game Speed", REG_DWORD},
	{"Music Volume", REG_DWORD},
	{"One Click Garrisoning", REG_DWORD},
	{"Rollover Text", REG_DWORD},
	{"Screen Size", REG_DWORD},
	{"Scroll Speed", REG_DWORD},
	{"Sound Volume", REG_DWORD},
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static void dump_values(HKEY root)
{
	LONG retval;
	const struct option *opt;
	unsigned i, n;
	DWORD type;
	char data[1024];
	DWORD lpcount;

	for (i = 0, n = ARRAY_SIZE(attr); i < n; ++i) {
		opt = &attr[i];

		type = opt->type;
		lpcount = sizeof data;

		retval = RegQueryValueEx(root, opt->name, NULL, &type, (LPBYTE)&data, &lpcount);
		if (retval != ERROR_SUCCESS) {
			fprintf(stderr, "Could not fetch \"%s\": %ld\n", opt->name, retval);
			return;
		}

		if (type != opt->type) {
			fprintf(stderr,
				"Bad type for \"%s\": expected %u but got %ld\n",
				opt->name, opt->type, type
			);
			return;
		}

		switch (opt->type) {
		case REG_DWORD:
			printf("%s: %X\n", opt->name, *((unsigned*)data));
			break;
		case REG_SZ:
			printf("%s: %s\n", opt->name, data);
			break;
		}
	}
}

int main(void)
{
	int error = 0;
	HKEY rootx = 0, root = 0;
	LONG retval;

	printf("Open rootx key \"%s\"\n", REG_XPATH);

	retval = RegOpenKeyEx(HKEY_CURRENT_USER, REG_XPATH, 0, KEY_READ, &rootx);
	if (retval != ERROR_SUCCESS)
		goto fail;

	printf("Open root key \"%s\"\n", REG_PATH);

	retval = RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &root);
	if (retval != ERROR_SUCCESS)
		goto fail;

	dump_values(root);

	error = 0;
fail:
	if (error)
		fputs("Failed to query registry\n", stderr);

	if (root)
		RegCloseKey(root);

	if (rootx)
		RegCloseKey(rootx);

	return error;
}
