/* Copyright 2018 Folkert van Verseveld. All rights resseved */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <linux/limits.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <libpe/pe.h>

#include "dbg.h"

#define TITLE "Age of Empires"

#define WIDTH 640
#define HEIGHT 480

unsigned init = 0;
SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

char path_cdrom[PATH_MAX];

#define BUFSZ 4096

#define STR_INSTALL_GAME 0x16
#define STR_RESET_GAME 0x1A
#define STR_NUKE_GAME 0x1B
#define STR_EXIT_SETUP 0x51
#define STR_OPEN_WEBSITE 0x3C

// scratch buffer
char buf[BUFSZ];

// 0.5 MiB of scratch virtual memory
#define SCRATCH_SIZE 524288
void *scratch;

static pe_ctx_t lib_lang;
static NODE_PERES *lib_lang_res;

void panic(const char *str) __attribute__((noreturn));

void *malloc_s(size_t n)
{
	void *ptr = malloc(n);
	if (!ptr)
		panic("Out of memory");
	return ptr;
}

// grabbed from peres.c demo
static void free_nodes(NODE_PERES *node)
{
	if (!node)
		return;

	while (node->nextNode)
		node = node->nextNode;

	while (node)
		if (!node->lastNode) {
			free(node);
			break;
		} else {
			node = node->lastNode;
			if (node->nextNode)
				free(node->nextNode);
		}
}

static NODE_PERES *createNode(NODE_PERES *currentNode, NODE_TYPE_PERES typeOfNextNode)
{
	assert(currentNode != NULL);
	NODE_PERES *newNode = malloc_s(sizeof(NODE_PERES));
	newNode->lastNode = currentNode;
	newNode->nextNode = NULL;
	newNode->nodeType = typeOfNextNode;
	currentNode->nextNode = newNode;
	return newNode;
}

static const NODE_PERES *lastNodeByTypeAndLevel(const NODE_PERES *currentNode, NODE_TYPE_PERES nodeTypeSearch, NODE_LEVEL_PERES nodeLevelSearch)
{
	assert(currentNode != NULL);
	if (currentNode->nodeType == nodeTypeSearch && currentNode->nodeLevel == nodeLevelSearch)
		return currentNode;

	while (currentNode != NULL) {
		currentNode = currentNode->lastNode;
		if (currentNode != NULL && currentNode->nodeType == nodeTypeSearch && currentNode->nodeLevel == nodeLevelSearch)
			return currentNode;
	}

	return NULL;
}

static NODE_PERES *pe_map_res(pe_ctx_t *ctx)
{
	const IMAGE_DATA_DIRECTORY * const resourceDirectory = pe_directory_by_entry(ctx, IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if (resourceDirectory == NULL || resourceDirectory->Size == 0)
		return NULL;

	uint64_t resourceDirOffset = pe_rva2ofs(ctx, resourceDirectory->VirtualAddress);
	uintptr_t offset = resourceDirOffset;
	void *ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
	if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DIRECTORY))) {
		panic("Internal error: pe_map_res");
		return NULL;
	}

	NODE_PERES *node = malloc_s(sizeof(NODE_PERES));
	node->lastNode = NULL; // root
	node->rootNode = NULL; // root
	node->nodeType = RDT_RESOURCE_DIRECTORY;
	node->nodeLevel = RDT_LEVEL1;
	node->resource.resourceDirectory = ptr;

	for (int i = 1, offsetDirectory1 = 0; i <= (lastNodeByTypeAndLevel(node, RDT_RESOURCE_DIRECTORY, RDT_LEVEL1)->resource.resourceDirectory->NumberOfNamedEntries +
												lastNodeByTypeAndLevel(node, RDT_RESOURCE_DIRECTORY, RDT_LEVEL1)->resource.resourceDirectory->NumberOfIdEntries); i++)
	{
		offsetDirectory1 += (i == 1) ? 16 : 8;
		offset = resourceDirOffset + offsetDirectory1;
		ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
		if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {
			// TODO: Should we report something?
			goto _error;
		}

		node = createNode(node, RDT_DIRECTORY_ENTRY);
		NODE_PERES *rootNode = node;
		node->rootNode = rootNode;
		node->nodeLevel = RDT_LEVEL1;
		node->resource.directoryEntry = ptr;

		const NODE_PERES * lastDirectoryEntryNodeAtLevel1 = lastNodeByTypeAndLevel(node, RDT_DIRECTORY_ENTRY, RDT_LEVEL1);

		if (lastDirectoryEntryNodeAtLevel1->resource.directoryEntry->DirectoryData.data.DataIsDirectory)
		{
			offset = resourceDirOffset + lastDirectoryEntryNodeAtLevel1->resource.directoryEntry->DirectoryData.data.OffsetToDirectory;
			ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
			if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DIRECTORY))) {
				// TODO: Should we report something?
				goto _error;
			}

			node = createNode(node, RDT_RESOURCE_DIRECTORY);
			node->rootNode = (NODE_PERES *)lastDirectoryEntryNodeAtLevel1;
			node->nodeLevel = RDT_LEVEL2;
			node->resource.resourceDirectory = ptr;
			//showNode(node);

			for (int j = 1, offsetDirectory2 = 0; j <= (lastNodeByTypeAndLevel(node, RDT_RESOURCE_DIRECTORY, RDT_LEVEL2)->resource.resourceDirectory->NumberOfNamedEntries +
					lastNodeByTypeAndLevel(node, RDT_RESOURCE_DIRECTORY, RDT_LEVEL2)->resource.resourceDirectory->NumberOfIdEntries); j++)
			{
				offsetDirectory2 += (j == 1) ? 16 : 8;
				offset = resourceDirOffset + lastNodeByTypeAndLevel(node, RDT_DIRECTORY_ENTRY, RDT_LEVEL1)->resource.directoryEntry->DirectoryData.data.OffsetToDirectory + offsetDirectory2;
				ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
				if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {
					// TODO: Should we report something?
					goto _error;
				}

				node = createNode(node, RDT_DIRECTORY_ENTRY);
				node->rootNode = rootNode;
				node->nodeLevel = RDT_LEVEL2;
				node->resource.directoryEntry = ptr;
				//showNode(node);

				offset = resourceDirOffset + node->resource.directoryEntry->DirectoryData.data.OffsetToDirectory; // posiciona em 0x72
				ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
				if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DIRECTORY))) {
					// TODO: Should we report something?
					goto _error;
				}

				node = createNode(node, RDT_RESOURCE_DIRECTORY);
				node->rootNode = rootNode;
				node->nodeLevel = RDT_LEVEL3;
				node->resource.resourceDirectory = ptr;
				//showNode(node);

				offset += sizeof(IMAGE_RESOURCE_DIRECTORY);

				for (int y = 1; y <= (lastNodeByTypeAndLevel(node, RDT_RESOURCE_DIRECTORY, RDT_LEVEL3)->resource.resourceDirectory->NumberOfNamedEntries +
									lastNodeByTypeAndLevel(node, RDT_RESOURCE_DIRECTORY, RDT_LEVEL3)->resource.resourceDirectory->NumberOfIdEntries); y++)
				{
					ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
					if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {
						// TODO: Should we report something?
						goto _error;
					}
					node = createNode(node, RDT_DIRECTORY_ENTRY);
					node->rootNode = rootNode;
					node->nodeLevel = RDT_LEVEL3;
					node->resource.directoryEntry = ptr;
					//showNode(node);

					offset = resourceDirOffset + node->resource.directoryEntry->DirectoryName.name.NameOffset;
					ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
					if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DATA_STRING))) {
						// TODO: Should we report something?
						goto _error;
					}
					node = createNode(node, RDT_DATA_STRING);
					node->rootNode = rootNode;
					node->nodeLevel = RDT_LEVEL3;
					node->resource.dataString = ptr;
					//showNode(node);

					offset = resourceDirOffset + node->lastNode->resource.directoryEntry->DirectoryData.data.OffsetToDirectory;
					ptr = LIBPE_PTR_ADD(ctx->map_addr, offset);
					if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DATA_ENTRY))) {
						// TODO: Should we report something?
						goto _error;
					}
					node = createNode(node, RDT_DATA_ENTRY);
					node->rootNode = rootNode;
					node->nodeLevel = RDT_LEVEL3;
					node->resource.dataEntry = ptr;
					//showNode(node);

					offset += sizeof(IMAGE_RESOURCE_DATA_ENTRY);
				}
			}
		}
	}

	return node;

_error:
	if (node != NULL)
		free_nodes(node);
	panic("Internal error");
	return NULL;
}

// XXX peres -x /media/methos/AOE/setupenu.dll

int find_lib_lang(char *path)
{
	char buf[PATH_MAX];
	struct stat st;

	snprintf(buf, PATH_MAX, "%s/setupenu.dll", path);
	if (stat(buf, &st))
		return 0;

	strcpy(path_cdrom, path);
	return 1;
}

int find_setup_files(void)
{
	char path[PATH_MAX];
	const char *user;
	DIR *dir;
	struct dirent *item;
	int found = 0;

	/*
	 * Try following paths in specified order:
	 * /media/cdrom
	 * /media/username/cdrom
	 * Finally, traverse every directory in /media/username
	 */

	if (find_lib_lang("/media/cdrom"))
		return 0;

	user = getlogin();
	snprintf(path, PATH_MAX, "/media/%s/cdrom", user);
	if (find_lib_lang(path))
		return 0;

	snprintf(path, PATH_MAX, "/media/%s", user);
	dir = opendir(path);
	if (!dir)
		return 0;
	
	errno = 0;
	while (item = readdir(dir)) {
		if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
			continue;

		snprintf(path, PATH_MAX, "/media/%s/%s", user, item->d_name);

		if (find_lib_lang(path)) {
			found = 1;
			break;
		}
	}

	closedir(dir);
	return found;
}

int load_lib_lang(void)
{
	snprintf(buf, BUFSZ, "%s/setupenu.dll", path_cdrom);
	if (pe_load_file(&lib_lang, buf) != LIBPE_E_OK)
		return 1;
	if (pe_parse(&lib_lang) != LIBPE_E_OK)
		return 1;
	if (!(lib_lang_res = pe_map_res(&lib_lang)))
		return 1;
	return 0;
}

#define RT_BITMAP 1
#define RT_STRING 5

#define MAX_PATH 256

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static const RESOURCE_ENTRY resource_types[] = {
	{ "RT_CURSOR",			1, ".cur",		"cursors"		},
	{ "RT_BITMAP",			2, ".bmp",		"bitmaps"		},
	{ "RT_ICON",			3, ".ico",		"icons"			},
	{ "RT_MENU",			4, ".rc",		"menus"			},
	{ "RT_DIALOG",			5, ".dlg",		"dialogs"		},
	{ "RT_STRING",			6, ".rc",		"strings"		},
	{ "RT_FONTDIR",			7, ".fnt",		"fontdirs"		},
	{ "RT_FONT",			8, ".fnt",		"fonts"			},
	{ "RT_ACCELERATOR",		9, ".rc",		"accelerators"	},
	{ "RT_RCDATA",			10, ".rc",		"rcdatas"		},
	{ "RT_MESSAGETABLE",	11, ".mc",		"messagetables"	},
	{ "RT_GROUP_CURSOR",	12, ".cur",		"groupcursors"	},
	{ "RT_GROUP_ICON",		14, ".ico",		"groupicons"	},
	{ "RT_VERSION",			16, ".rc",		"versions"		},
	{ "RT_DLGINCLUDE",		17, ".rc",		"dlgincludes"	},
	{ "RT_PLUGPLAY",		19, ".rc",		"plugplays"		},
	{ "RT_VXD",				20, ".rc",		"xvds"			},
	{ "RT_ANICURSOR",		21, ".rc",		"anicursors"	},
	{ "RT_ANIICON",			22, ".rc",		"aniicons"		},
	{ "RT_HTML",			23, ".html",	"htmls"			},
	{ "RT_MANIFEST",		24, ".xml",		"manifests"		},
	{ "RT_DLGINIT",			240, ".rc",		"dlginits"		},
	{ "RT_TOOLBAR",			241, ".rc",		"toolbars"		}
};

static const RESOURCE_ENTRY *getResourceEntryByNameOffset(uint32_t nameOffset)
{
	for (size_t i = 0; i < ARRAY_SIZE(resource_types); i++)
		if (resource_types[i].nameOffset == nameOffset)
			return &resource_types[i];

	return NULL;
}

static void pe_node_get_path(pe_ctx_t *ctx, const NODE_PERES *node, char *path)
{
	for (unsigned level = RDT_LEVEL1; level <= node->nodeLevel; level++) {
		char name[MAX_PATH];

		const NODE_PERES *parent = lastNodeByTypeAndLevel(node, RDT_DIRECTORY_ENTRY, level);
		if (parent->resource.directoryEntry->DirectoryName.name.NameIsString) {

			const IMAGE_DATA_DIRECTORY * const resourceDirectory = pe_directory_by_entry(ctx, IMAGE_DIRECTORY_ENTRY_RESOURCE);
			if (resourceDirectory == NULL || resourceDirectory->Size == 0)
				return;

			const uint64_t offsetString = pe_rva2ofs(ctx, resourceDirectory->VirtualAddress + parent->resource.directoryEntry->DirectoryName.name.NameOffset);
			const IMAGE_RESOURCE_DATA_STRING *ptr = LIBPE_PTR_ADD(ctx->map_addr, offsetString);

			if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DATA_STRING))) {
				// TODO: Should we report something?
				return;
			}
			const uint16_t stringSize = ptr->length;
			if (stringSize + 2 <= MAX_PATH){
				// quick & dirty UFT16 to ASCII conversion
				for (uint16_t p = 0; p <= stringSize*2; p += 2){
					memcpy(name + p/2, (char*)(ptr->string) + p, 1);
				}
				strncpy(name + stringSize, " \0", 2);
			}
		} else {
			const RESOURCE_ENTRY *resourceEntry;
			if (level == RDT_LEVEL1 && (resourceEntry = getResourceEntryByNameOffset(parent->resource.directoryEntry->DirectoryName.name.NameOffset))) {
				snprintf(name, MAX_PATH, "%s ", resourceEntry->name);
			} else
				snprintf(name, MAX_PATH, "%04x ", parent->resource.directoryEntry->DirectoryName.name.NameOffset);
		}
		strncat(path, name, MAX_PATH - strlen(path));
	}
	path[strlen(path)-1] = 0;
}

static void pe_dump_data_entry(pe_ctx_t *ctx, const NODE_PERES *node)
{
	char path[MAX_PATH];
	path[0] = '\0';

	pe_node_get_path(ctx, node, path);
	printf("%s (%d bytes)\n", path, node->resource.dataEntry->size);
}

#define PE_DATA_ENTRY_NAMESZ 64

struct pe_data_entry {
	char name[PE_DATA_ENTRY_NAMESZ];
	unsigned type;
	unsigned offset;
	unsigned id;
	void *data;
	unsigned size;
};

#define MAX_MSG 4096

static void showNode(const NODE_PERES *node)
{
	char value[MAX_MSG];

	switch (node->nodeType)
	{
		default:
			dbgf("ShowNode: %s\n", "ERROR - Invalid Node Type");
			break;
		case RDT_RESOURCE_DIRECTORY:
		{
			const IMAGE_RESOURCE_DIRECTORY * const resourceDirectory = node->resource.resourceDirectory;

			snprintf(value, MAX_MSG, "Resource Directory / %d", node->nodeLevel);
			dbgf("\nNode Type / Level: %s\n", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->Characteristics);
			dbgf("Characteristics: %s\n", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->TimeDateStamp);
			dbgf("Timestamp: %s\n", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->MajorVersion);
			dbgf("Major Version: %s\n", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->MinorVersion);
			dbgf("Minor Version: %s\n", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->NumberOfNamedEntries);
			dbgf("Named entries: %s\n", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->NumberOfIdEntries);
			dbgf("Id entries: %s\n", value);
			break;
		}
		case RDT_DIRECTORY_ENTRY:
		{
			const IMAGE_RESOURCE_DIRECTORY_ENTRY * const directoryEntry = node->resource.directoryEntry;

			snprintf(value, MAX_MSG, "Directory Entry / %d", node->nodeLevel);
			dbgf("\nNode Type / Level: %s\n", value);

			snprintf(value, MAX_MSG, "%d", directoryEntry->DirectoryName.name.NameOffset);
			dbgf("Name offset: %s\n", value);

			snprintf(value, MAX_MSG, "%d", directoryEntry->DirectoryName.name.NameIsString);
			dbgf("Name is string: %s\n", value);

			snprintf(value, MAX_MSG, "%x", directoryEntry->DirectoryData.data.OffsetToDirectory);
			dbgf("Offset to directory: %s\n", value);

			snprintf(value, MAX_MSG, "%d", directoryEntry->DirectoryData.data.DataIsDirectory);
			dbgf("Data is directory: %s\n", value);
			break;
		}
		case RDT_DATA_STRING:
		{
			const IMAGE_RESOURCE_DATA_STRING * const dataString = node->resource.dataString;

			snprintf(value, MAX_MSG, "Data String / %d", node->nodeLevel);
			dbgf("\nNode Type / Level: %s\n", value);

			snprintf(value, MAX_MSG, "%d", dataString->length);
			dbgf("String len: %s\n", value);

			snprintf(value, MAX_MSG, "%d", dataString->string[0]);
			dbgf("String: %s\n", value);
			break;
		}
		case RDT_DATA_ENTRY:
		{
			const IMAGE_RESOURCE_DATA_ENTRY * const dataEntry = node->resource.dataEntry;

			snprintf(value, MAX_MSG, "Data Entry / %d", node->nodeLevel);
			dbgf("\nNode Type / Level: %s\n", value);

			snprintf(value, MAX_MSG, "%x", dataEntry->offsetToData);
			dbgf("OffsetToData: %s\n", value);

			snprintf(value, MAX_MSG, "%d", dataEntry->size);
			dbgf("Size: %s\n", value);

			snprintf(value, MAX_MSG, "%d", dataEntry->codePage);
			dbgf("CodePage: %s\n", value);

			snprintf(value, MAX_MSG, "%d", dataEntry->reserved);
			dbgf("Reserved: %s\n", value);
			break;
		}
	}
}

static int pe_find_data_entry(pe_ctx_t *ctx, const NODE_PERES *node, struct pe_data_entry *entry, unsigned type, unsigned id)
{
	unsigned entry_type = 0;
	unsigned offset;
	const RESOURCE_ENTRY *resourceEntry;

	char name[MAX_PATH];

	for (unsigned level = RDT_LEVEL1; level <= node->nodeLevel; ++level) {
		const NODE_PERES *parent = lastNodeByTypeAndLevel(node, RDT_DIRECTORY_ENTRY, level);
		if (parent->resource.directoryEntry->DirectoryName.name.NameIsString)
		{
			const IMAGE_DATA_DIRECTORY * const resourceDirectory = pe_directory_by_entry(ctx, IMAGE_DIRECTORY_ENTRY_RESOURCE);
			if (resourceDirectory == NULL || resourceDirectory->Size == 0)
				return 0;

			const uint64_t offsetString = pe_rva2ofs(ctx, resourceDirectory->VirtualAddress + parent->resource.directoryEntry->DirectoryName.name.NameOffset);
			const IMAGE_RESOURCE_DATA_STRING *ptr = LIBPE_PTR_ADD(ctx->map_addr, offsetString);

			if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DATA_STRING)))
				// TODO: Should we report something?
				return 0;

			const uint16_t stringSize = ptr->length;
			if (stringSize + 2 <= MAX_PATH){
				// quick & dirty UFT16 to ASCII conversion
				for (uint16_t p = 0; p <= stringSize*2; p += 2){
					memcpy(name + p/2, (char*)(ptr->string) + p, 1);
				}
				strncpy(name + stringSize, " \0", 2);
			}
		}

		if (level == RDT_LEVEL1 && (resourceEntry = getResourceEntryByNameOffset(parent->resource.directoryEntry->DirectoryName.name.NameOffset))) {
			entry_type = (unsigned)(resourceEntry - resource_types);
			//printf("%s ", resourceEntry->name);
		} else {
			//dbgf("lvl: %u\n", level - RDT_LEVEL1 + 1);
			offset = parent->resource.directoryEntry->DirectoryName.name.NameOffset;
			//printf("%04x ", offset);
			// string are stored in blocks of 16
			if (type == RT_STRING) {
				if (id >= (offset - 1) * 16 && id < offset * 16)
					goto found;
			} else if (offset == id)
				goto found;
		}
	}

	return 0;
found:
	strncpy(entry->name, name, PE_DATA_ENTRY_NAMESZ);
	entry->type = entry_type;
	entry->offset = offset;
	entry->id = id;
	//showNode(node);

	return 1;
}

static int pe_load_data_entry(pe_ctx_t *ctx, const NODE_PERES *node, struct pe_data_entry *entry, unsigned type, unsigned id)
{
	while (node->lastNode)
		node = node->lastNode;

	while (node) {
		if (node->nodeType == RDT_DATA_ENTRY)
			if (pe_find_data_entry(&lib_lang, node, entry, type, id)) {
				dbgs("found:");
				if (!entry->type)
					dbgf("name: %s\n", entry->name);
				dbgf("type: %u\n%04X,%04X\n", entry->type, entry->offset, entry->id);
				const IMAGE_RESOURCE_DATA_ENTRY *data_entry = node->resource.dataEntry;
				uint64_t offsetData = pe_rva2ofs(ctx, data_entry->offsetToData);
				entry->data = LIBPE_PTR_ADD(ctx->map_addr, offsetData);
				entry->size = data_entry->size;
				return 0;
			}
		node = node->nextNode;
	}
	return 1;
}

int load_string(unsigned id, char *str, size_t size)
{
	struct pe_data_entry entry;
	uint16_t *data;
	dbgf("load_string %04X\n", id);
	if (!pe_load_data_entry(&lib_lang, lib_lang_res, &entry, RT_STRING, id)) {
		data = entry.data;
		for (unsigned i = 0, n = id % 16; i < n; ++i)
			data += *data + 1;
		str[size - 1] = '\0';
		// FIXME quick and dirty UTF16 to ASCII conversion
		size_t i, n;
		for (i = 0, n = *data; i < size && i < n; ++i)
			str[i] = data[i + 1];
		if (i < size)
			str[i] = '\0';
		dbgf("str: %s\n", str);
		return 0;
	}
	return 1;
}

struct bmp_header {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} __attribute__((packed));

struct img_header {
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};

// NOTE strictly assumes file is at least 40 bytes
uint32_t img_pixel_offset(const void *data, size_t n)
{
	uint32_t offset = 0;
	const struct img_header *hdr = data;

	if (n < 40 || hdr->biSize >= n || hdr->biBitCount != 8)
		return 0;

	// assume all entries are used
	return hdr->biSize + 256 * sizeof(uint32_t);
}

// FIXME remove this when load_bitmap works
int is_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	return n >= sizeof *hdr && hdr->bfType == 0x4D42;
}

int is_img(const void *data, size_t n)
{
	const struct img_header *hdr = data;
	return n >= 12 && hdr->biSize < n;
}

uint8_t *dump_coltbl(const void *data, size_t size, unsigned bits)
{
	size_t n = 1 << bits;
	if (n * sizeof(uint32_t) > size) {
		fputs("corrupt color table\n", stderr);
		return NULL;
	}
#if 1
	return (uint8_t*)((uint32_t*)data + n);
#else
	uint32_t *col = data;
	for (size_t i = 0; i < n; ++i)
		printf("%8X: %8X\n", i, col[i]);
	return (uint8_t*)&col[n];
#endif
}

void dump_img(const void *data, size_t n, size_t offset)
{
	const struct img_header *hdr = data;
	if (n < 12 || hdr->biSize >= n) {
		fputs("corrupt img header\n", stderr);
		return;
	}
	dbgf("img header:\nbiSize: 0x%X\nbiWidth: %u\nbiHeight: %u\n", hdr->biSize, hdr->biWidth, hdr->biHeight);
	if (hdr->biSize < 12)
		return;
	dbgf("biPlanes: %u\nbiBitCount: %u\nbiCompression: %u\n", hdr->biPlanes, hdr->biBitCount, hdr->biCompression);
	dbgf("biSizeImage: 0x%X\n", hdr->biSizeImage);
	dbgf("biXPelsPerMeter: %u\nbiYPelsPerMeter: %u\n", hdr->biXPelsPerMeter, hdr->biYPelsPerMeter);
	dbgf("biClrUsed: %u\nbiClrImportant: %u\n", hdr->biClrUsed, hdr->biClrImportant);
	if (hdr->biBitCount <= 8) {
		uint8_t *end = dump_coltbl((const unsigned char*)data + hdr->biSize, n - hdr->biSize, hdr->biBitCount);
		dbgf("pixel data start: 0x%zX\n", offset + hdr->biSize + 256 * sizeof(uint32_t));
		if (end && end + hdr->biHeight * hdr->biWidth > (const unsigned char*)data + n)
			fputs("truncated pixel data\n", stderr);
	}
}

void dump_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	if (!is_bmp(data, n)) {
		if (!is_img(data, n))
			fputs("corrupt bmp file\n", stderr);
		else
			dump_img(data, n, 0);
		return;
	}
	dbgf("bmp header:\nbfType: 0x%04X\nSize: 0x%X\n", hdr->bfType, hdr->bfSize);
	dbgf("Start pixeldata: 0x%X\n", hdr->bfOffBits);
	dump_img(hdr + 1, n - sizeof *hdr, sizeof *hdr);
}

// FIXME also support big endian machines
int load_bitmap(unsigned id, void **data, size_t *size)
{
	struct pe_data_entry entry;
	struct bmp_header *hdr;

	dbgf("load_bitmap %04X\n", id);
	if (pe_load_data_entry(&lib_lang, lib_lang_res, &entry, RT_BITMAP, id))
		return 1;
	if (entry.size + sizeof *hdr > SCRATCH_SIZE)
		panic("bitmap too big");
	// TODO reconstruct bmp_header in scratch mem
	uint32_t offset = img_pixel_offset(entry.data, entry.size);
	if (!offset)
		return 1;
	hdr = scratch;
	hdr->bfType = 0x4D42;
	hdr->bfSize = entry.size + sizeof *hdr;
	hdr->bfReserved1 = hdr->bfReserved2 = 0;
	hdr->bfOffBits = offset + sizeof *hdr;

	memcpy(hdr + 1, entry.data, entry.size);
	dump_bmp(hdr, hdr->bfSize);

	*data = entry.data;
	*size = entry.size + sizeof *hdr;

	return 0;
}

#define PANIC_BUFSZ 1024

void panic(const char *str)
{
	char buf[PANIC_BUFSZ];
	snprintf(buf, PANIC_BUFSZ, "zenity --error --text=\"%s\"", str);
	system(buf);
	exit(1);
}

SDL_Surface *surf_start, *surf_reset, *surf_nuke, *surf_exit, *surf_website;
SDL_Texture *tex_start, *tex_reset, *tex_nuke, *tex_exit, *tex_website;

SDL_Surface *surf_bkg, *surf_btn;
SDL_Texture *tex_bkg, *tex_btn;

struct menu_item {
	SDL_Rect pos;
	int x, y;
	unsigned image;
	unsigned id, format;
	SDL_Surface **surf;
	SDL_Texture **tex;
} menu_items[] = {
	{{0xf1, 0x90 , 0x1b8, 0xb7 }, 197, 138, 2, STR_INSTALL_GAME, 0x10, &surf_start, &tex_start},
	{{0xf1, 0xba , 0x1b0, 0xcd }, 197, 180, 0, STR_RESET_GAME, 0, &surf_reset, &tex_reset},
	{{0xf1, 0xe6 , 0x1b1, 0xf9 }, 197, 223, 0, STR_NUKE_GAME, 0, &surf_nuke, &tex_nuke},
	{{0xf1, 0x10f, 0x1b8, 0x136}, 197, 265, 1, STR_EXIT_SETUP, 0x10, &surf_exit, &tex_exit},
	{{0xf1, 0x13a, 0x1b8, 0x161}, 197, 307, 1, STR_OPEN_WEBSITE, 0x10, &surf_website, &tex_website},
};

void init_main_menu(void)
{
	SDL_Color fg = {0, 0, 0, 255};

	// FIXME proper error handling
	load_string(STR_INSTALL_GAME, buf, BUFSZ);
	surf_start = TTF_RenderText_Solid(font, buf, fg);
	load_string(STR_RESET_GAME, buf, BUFSZ);
	surf_reset = TTF_RenderText_Solid(font, buf, fg);
	load_string(STR_NUKE_GAME, buf, BUFSZ);
	surf_nuke = TTF_RenderText_Solid(font, buf, fg);
	load_string(STR_EXIT_SETUP, buf, BUFSZ);
	surf_exit = TTF_RenderText_Solid(font, buf, fg);
	load_string(STR_OPEN_WEBSITE, buf, BUFSZ);
	surf_website = TTF_RenderText_Solid(font, buf, fg);

	tex_start = SDL_CreateTextureFromSurface(renderer, surf_start);
	assert(tex_start);
	tex_reset = SDL_CreateTextureFromSurface(renderer, surf_reset);
	assert(tex_reset);
	tex_nuke = SDL_CreateTextureFromSurface(renderer, surf_nuke);
	assert(tex_nuke);
	tex_exit = SDL_CreateTextureFromSurface(renderer, surf_exit);
	assert(tex_exit);
	tex_website = SDL_CreateTextureFromSurface(renderer, surf_website);
	assert(tex_website);

	void *data;
	size_t size;
	int ret;
	SDL_RWops *mem;

	ret = load_bitmap(0xA2, &data, &size);
	assert(ret == 0);

	mem = SDL_RWFromMem(scratch, size);
	surf_bkg = SDL_LoadBMP_RW(mem, 1);
	tex_bkg = SDL_CreateTextureFromSurface(renderer, surf_bkg);
	assert(tex_bkg);

	ret = load_bitmap(0xD1, &data, &size);
	assert(ret == 0);
	mem = SDL_RWFromMem(scratch, size);
	surf_btn = SDL_LoadBMP_RW(mem, 1);
	// FIXME remove transparent pixels
	printf("format: %X\n", SDL_PIXELTYPE(surf_btn->format->format));
	tex_btn = SDL_CreateTextureFromSurface(renderer, surf_btn);
	assert(tex_btn);
}

void display_main_menu(void)
{
	SDL_Rect pos, img;
	pos.x = 0; pos.y = 0;
	pos.w = surf_bkg->w; pos.h = surf_bkg->h;
	SDL_RenderCopy(renderer, tex_bkg, NULL, &pos);

	for (unsigned i = 0; i < ARRAY_SIZE(menu_items); ++i) {
		struct menu_item *item = &menu_items[i];
		pos.x = item->x; pos.y = item->y;
		pos.w = surf_btn->w; pos.h = surf_btn->h / 4;
		img.x = 0; img.y = item->image * surf_btn->h / 4;
		img.w = surf_btn->w; img.h = surf_btn->h / 4;
		SDL_RenderCopy(renderer, tex_btn, &img, &pos);
	}

	for (unsigned i = 0; i < ARRAY_SIZE(menu_items); ++i) {
		struct menu_item *item = &menu_items[i];
		item->pos.w = (*item->surf)->w;
		item->pos.h = (*item->surf)->h;
		SDL_RenderCopy(renderer, *item->tex, NULL, &item->pos);
	}
}

void main_event_loop(void)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	display_main_menu();
	SDL_RenderPresent(renderer);

	SDL_Event ev;
	while (SDL_WaitEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
		case SDL_KEYDOWN:
			return;
		}
	}
}

int main(void)
{
	if (!find_setup_files())
		panic("Please insert or mount the game CD-ROM");
	if (load_lib_lang())
		panic("CD-ROM files are corrupt");

	scratch = mmap(NULL, SCRATCH_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (scratch == MAP_FAILED)
		panic("Could not initialize scratch memory");

	if (SDL_Init(SDL_INIT_VIDEO))
		panic("Could not initialize user interface");

	if (!(window = SDL_CreateWindow(
		TITLE, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
		SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN)))
	{
		panic("Could not create user interface");
	}

	dbgf("Available render drivers: %d\n", SDL_GetNumVideoDrivers());

	// Create default renderer and don't care if it is accelerated.
	if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC)))
		panic("Could not create rendering context");

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		panic("Could not initialize image subsystem");
	if (TTF_Init())
		panic("Could not initialize fonts");

	snprintf(buf, BUFSZ, "%s/system/fonts/arial.ttf", path_cdrom);
	font = TTF_OpenFont(buf, 20);
	if (!font)
		panic("Could not setup font");

	init_main_menu();
	main_event_loop();

	TTF_Quit();
	IMG_Quit();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
