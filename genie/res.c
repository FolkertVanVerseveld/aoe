/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * PE resource wrapper API
 *
 * peres.c portions are licensed under General Public License v3.0
 */

#include <genie/res.h>

#include <genie/bmp.h>
#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/memory.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pe.h"

#ifndef _WIN32

static void *scratch;
static size_t scratch_size;

#define PE_DATA_ENTRY_NAMESZ 64

struct pe_data_entry {
	char name[PE_DATA_ENTRY_NAMESZ];
	unsigned type;
	unsigned offset;
	unsigned id;
	void *data;
	unsigned size;
};

static size_t nextpow2(size_t v)
{
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	return ++v;
}

static void scratch_resize(size_t n)
{
	void *ptr;

	if (scratch && scratch_size >= n)
		return;

	n = nextpow2(n);
	ptr = realloc(scratch, n);
	if (!ptr)
		panic("Out of memory");
	scratch = ptr;
	scratch_size = n;
}

static void *malloc_s(size_t n)
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

// grabbed from peres.c demo
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

// grabbed from peres.c demo
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

// grabbed from peres.c demo
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

#define MAX_PATH 256

// grabbed from peres.c demo
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

#define MAX_MSG 4096

#if 0
// grabbed from peres.c demo
static void showNode(const NODE_PERES *node)
{
	char value[MAX_MSG];

	switch (node->nodeType) {
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
#endif

// grabbed from peres.c demo
static int pe_find_data_entry(pe_ctx_t *ctx, const NODE_PERES *node, struct pe_data_entry *entry, unsigned type, unsigned id)
{
	unsigned entry_type = 0;
	unsigned offset;
	const RESOURCE_ENTRY *resourceEntry;

	char name[MAX_PATH];

	for (unsigned level = RDT_LEVEL1; level <= node->nodeLevel; ++level) {
		const NODE_PERES *parent = lastNodeByTypeAndLevel(node, RDT_DIRECTORY_ENTRY, level);
		if (parent->resource.directoryEntry->DirectoryName.name.NameIsString) {
			const IMAGE_DATA_DIRECTORY * const resourceDirectory = pe_directory_by_entry(ctx, IMAGE_DIRECTORY_ENTRY_RESOURCE);
			if (resourceDirectory == NULL || resourceDirectory->Size == 0)
				return 0;

			const uint64_t offsetString = pe_rva2ofs(ctx, resourceDirectory->VirtualAddress + parent->resource.directoryEntry->DirectoryName.name.NameOffset);
			const IMAGE_RESOURCE_DATA_STRING *ptr = LIBPE_PTR_ADD(ctx->map_addr, offsetString);

			if (!pe_can_read(ctx, ptr, sizeof(IMAGE_RESOURCE_DATA_STRING)))
				// TODO: Should we report something?
				return 0;

			const uint16_t stringSize = ptr->length;
			if (stringSize + 2 <= MAX_PATH) {
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
			if (pe_find_data_entry(ctx, node, entry, type, id)) {
				//dbgs("found:");
				if (!entry->type)
					;//dbgf("name: %s\n", entry->name);
				//dbgf("type: %u\n%04X,%04X\n", entry->type, entry->offset, entry->id);
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

int pe_lib_open(struct pe_lib *lib, const char *name)
{
	if (pe_load_file(&lib->ctx, name) != LIBPE_E_OK)
		return 1;
	if (pe_parse(&lib->ctx) != LIBPE_E_OK)
		return 1;
	if (!(lib->res = pe_map_res(&lib->ctx)))
		return 1;

	// now side-load our own implementation
	lib->pe = mem_alloc(sizeof(struct pe));
	pe_init(lib->pe);
	return pe_open(lib->pe, name);
}

void pe_lib_close(struct pe_lib *lib)
{
	pe_unload(&lib->ctx);
	pe_close(lib->pe);
	mem_free(lib->pe);
}

// NOTE strictly assumes file is at least 40 bytes
uint32_t img_pixel_offset(const void *data, size_t n)
{
	const struct img_header *hdr = data;

	if (n < 40 || hdr->biSize >= n || hdr->biBitCount != 8)
		return 0;

	// assume all entries are used
	return hdr->biSize + 256 * sizeof(uint32_t);
}

// XXX copied from bmp.c

/** Validate bitmap header. */
int is_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	return n >= sizeof *hdr && hdr->bfType == BMP_BF_TYPE;
}

/** Validate bitmap image header. */
int is_img(const void *data, size_t n)
{
	const struct img_header *hdr = data;
	return n >= 12 && hdr->biSize < n;
}

// END copied from bmp.c

/** Fetch color table. */
static uint8_t *get_coltbl(const void *data, size_t size, unsigned bits)
{
	size_t n = 1 << bits;
	if (n * sizeof(uint32_t) > size) {
		fputs("corrupt color table\n", stderr);
		return NULL;
	}
	return (uint8_t*)((uint32_t*)data + n);
}

static void dump_img(const void *data, size_t n, size_t offset)
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
		uint8_t *end = get_coltbl((const unsigned char*)data + hdr->biSize, n - hdr->biSize, hdr->biBitCount);
		dbgf("pixel data start: 0x%zX\n", offset + hdr->biSize + 256 * sizeof(uint32_t));
		if (end && end + hdr->biHeight * hdr->biWidth > (const unsigned char*)data + n)
			fputs("truncated pixel data\n", stderr);
	}
}

static void dump_bmp(const void *data, size_t n)
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

int load_string(struct pe_lib *lib, unsigned id, char *str, size_t size)
{
	struct pe_data_entry entry;
	uint16_t *data;
	//dbgf("load_string %04X\n", id);
	if (pe_load_data_entry(&lib->ctx, lib->res, &entry, RT_STRING, id))
		return 1;

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
	else
		str[size - 1] = '\0';
	//dbgf("str: %s\n", str);

	// now try our own version and compare
	char cmp[4096];
	pe_load_string(id, cmp, sizeof cmp);

	if (strcmp(str, cmp))
		fprintf(stderr, "diff:\n%s\n%s", str, cmp);

	return 0;
}

// FIXME also support big endian machines
int load_bitmap(struct pe_lib *lib, unsigned id, void **data, size_t *size)
{
	struct pe_data_entry entry;
	struct bmp_header *hdr;

	dbgf("load_bitmap %04X\n", id);
	if (pe_load_data_entry(&lib->ctx, lib->res, &entry, RT_BITMAP, id))
		return 1;
	scratch_resize(entry.size + sizeof *hdr);
	// reconstruct bmp_header in scratch mem
	uint32_t offset = img_pixel_offset(entry.data, entry.size);
	if (!offset)
		return 1;
	hdr = scratch;
	hdr->bfType = BMP_BF_TYPE;
	hdr->bfSize = entry.size + sizeof *hdr;
	hdr->bfReserved1 = hdr->bfReserved2 = 0;
	hdr->bfOffBits = offset + sizeof *hdr;

	memcpy(hdr + 1, entry.data, entry.size);
	dump_bmp(hdr, hdr->bfSize);

	*data = hdr;
	*size = entry.size + sizeof *hdr;

	return 0;
}

#else
// win32 implementation here...
	#include <windows.h>

int pe_lib_open(struct pe_lib *lib, const char *name)
{
	lib->module = LoadLibrary(name);
	if (!lib->module) {
		fprintf(stderr, "pe_lib_open: failed for %s\n", name);
		return 1;
	}
	return 0;
}

void pe_lib_close(struct pe_lib *lib)
{
	CloseHandle(lib->module);
}

int load_string(struct pe_lib *lib, unsigned id, char *str, size_t size)
{
	return LoadString(lib->module, id, str, size) == 0;
}
#endif
