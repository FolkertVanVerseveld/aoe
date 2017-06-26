/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/*
quick and dirty program that tries to extract all strings from string
table from a windows dynamic load library. you need to pipe the output
to a file.
*/
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define error(msg) do{perror(msg);exit(EXIT_FAILURE);}while(0)

struct res_hdr {
	uint32_t DataSize;
	uint32_t HeaderSize;
	uint32_t TYPE, NAME;
	uint32_t DataVersion;
	uint16_t MemoryFlags;
	uint16_t LanguageId;
	uint32_t Version;
	uint32_t Characteristics;
};

struct msg_res_blk {
	uint32_t LowId, HighId;
	uint32_t OffsetToEntries;
};

static void res_hdr_read(struct res_hdr *res, uint8_t *data) {
	memcpy(res, data, sizeof(struct res_hdr));
}

#define RT_UNKNOWN 0
#define RT_CURSOR 1
#define RT_BITMAP 2
#define RT_ICON 3
#define RT_MENU 4
#define RT_DIALOG 5
#define RT_STRING 6
#define RT_FONTDIR 7
#define RT_FONT 8
#define RT_ACCELERATOR 9
#define RT_RCDATA 10
#define RT_MESSAGETABLE 11
#define RT_GROUP_CURSOR 12
#define RT_GROUP_ICON 14
#define RT_VERSION 16
#define RT_DLGINCLUDE 17
#define RT_PLUGPLAY 19
#define RT_VXD 20
#define RT_ANICURSOR 21
#define RT_ANIICON 22

const char *rtbl[] = {
	[RT_CURSOR      ]   = "cursor",
	[RT_BITMAP      ]   = "bitmap",
	[RT_ICON        ]   = "icon",
	[RT_MENU        ]   = "menu",
	[RT_DIALOG      ]   = "dialog",
	[RT_STRING      ]   = "string",
	[RT_FONTDIR     ]   = "fontdir",
	[RT_FONT        ]   = "font",
	[RT_ACCELERATOR ]   = "accelerator",
	[RT_RCDATA      ]   = "rcdata",
	[RT_MESSAGETABLE]   = "messagetable",
	[RT_GROUP_CURSOR]   = "group cursor",
	[RT_GROUP_ICON  ]   = "group icon",
	[RT_VERSION     ]   = "version",
	[RT_DLGINCLUDE  ]   = "dlginclude",
	[RT_PLUGPLAY    ]   = "plugpray",
	[RT_VXD         ]   = "vxd",
	[RT_ANICURSOR   ]   = "anicursor",
	[RT_ANIICON     ]   = "aniicon"
};

#define rt_type(x) ((x)>>16)

static void res_hdr_dump(const struct res_hdr *res) {
	const char *type = rtbl[0];
	if (rt_type(res->TYPE) <= RT_ANIICON) {
		printf("type=%u\n", rt_type(res->TYPE));
		type = rtbl[rt_type(res->TYPE)];
	}
	printf(
		"DataSize: %x\n"
		"HeaderSize: %x\n"
		"TYPE: %x (%s)\n"
		"NAME: %x\n"
		"DataVersion: %x\n"
		"MemoryFlags: %hx\n"
		"LanguageId: %hx\n"
		"Version: %x\n"
		"Characteristics: %x\n",
		res->DataSize,
		res->HeaderSize,
		res->TYPE, type ? type : "???",
		res->NAME,
		res->DataVersion,
		res->MemoryFlags,
		res->LanguageId,
		res->Version,
		res->Characteristics
	);
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fputs("usage: str file\n", stderr);
		return 1;
	}
	int fd = open(argv[1], O_RDONLY);
	struct stat sb;
	if (fd == -1) error("open");
	if (fstat(fd, &sb) == -1) error("stat");
	char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) error("mmap");
	uint16_t *dw = (uint16_t*)addr;
	uint8_t  *db = (uint8_t *)addr, *end = (uint8_t*)addr + sb.st_size;
	size_t res_count = 0;
	struct res_hdr res;
	do {
		uint8_t *end;
		res_hdr_read(&res, db);
		++res_count;
		res_hdr_dump(&res);
		db += res.HeaderSize;
		end = db + res.DataSize;
		while (db < end) {
			dw = (uint16_t*)db;
			printf("length: %u\n", *dw);
			uint16_t *str, l;
			putchar('"');
			for (str = (uint16_t*)(db + 2), l = *dw; l > 0; --l, ++str) {
				if (isprint(*str)) putchar(*str);
				else printf("\\u%04u", *str);
			}
			puts("\"");
			db += 2 + 2 * *dw;
		}
		db = end;
	} while (db < end);
	res_hdr_dump(&res);
	printf("resource count: %zu\n", res_count);
	close(fd);
	return 0;
}
