/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "pe.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>

#include <genie/dbg.h>

#include <xt/string.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/*
 * BYTE = 8 bits
 * WORD = 16 bits
 * DWORD = 32 bits
 * QWORD = 64 bits
 */

struct pestr {
	uint16_t length;
	char data[];
};

static inline size_t peopthdr_size(const union peopthdr *opt)
{
	return opt->hdr32.coff.o_magic == 0x10b ? sizeof(struct peopthdr32) : sizeof(struct peopthdr64);
}

static inline uint32_t peopthdr_rva_count(const union peopthdr *opt)
{
	return opt->hdr32.coff.o_magic == 0x10b ? opt->hdr32.o_nrvasz : opt->hdr64.o_nrvasz;
}

static inline size_t penthdr_size(const struct penthdr *nt)
{
	return sizeof *nt - sizeof(union peopthdr) + peopthdr_size(&nt->OptionalHeader);
}

void pe_init(struct pe *pe)
{
	fs_blob_init(&pe->blob);

	pe->type = XT_UNKNOWN;
	pe->bits = 0;
	pe->mz = NULL;
	pe->dos = NULL;
	pe->peopt = NULL;
	pe->nrvan = pe->nrvasz = 0;
	pe->sec = NULL;
	pe->sec_rsrc = NULL;
	pe->rsrc = NULL;
}

int pe_open(struct pe *this, const char *path)
{
	/*
	 * Incrementally figure out the file format. We start with the
	 * ol' MS-DOS format and gradually build up to winnt PE.
	 */
	int err;

	err = fs_blob_open(&this->blob, path, FS_MODE_READ | FS_MODE_MAP);
	if (err)
		return err;

	const char *data = this->blob.data;
	size_t size = this->blob.size;
	const char *end = data + size;

	if (size < sizeof(struct mz)) {
		fputs("pe: bad dos header: file too small\n", stderr);
		return 1;
	}

	// probably COM file
	this->type = XT_MZ;
	this->bits = 16;

	struct mz *mz = this->blob.data;
	this->mz = mz;

	// verify fields
	if (mz->e_magic != DOS_MAGIC) {
		xtfprintf(stderr, "pe: bad dos magic: got %I16X, %I16X expected\n", mz->e_magic, DOS_MAGIC);
		return 1;
	}

	size_t start = mz->e_cparhdr * 16;

	if (start >= size) {
		xtfprintf(stderr, "pe: bad exe start: got %zu, max: %zu\n", start, size - 1);
		return 1;
	}

	off_t data_start = mz->e_cp * 512;

	if (mz->e_cblp) {
		data_start -= 512 - mz->e_cblp;
		if (data_start < 0) {
			xtfprintf(stderr, "pe: bad data_start: -%zd\n", data_start);
			return 1;
		}
	}

	dbgf("pe: data start: %" PRIX64 "\n", data_start);

	// cannot be winnt if program entry is within dos block
	if (start < sizeof(struct dos)) {
		printf("dos start: %zX\n", start);
		return 0;
	}

	if (start >= size) {
		fputs("bad dos start: file too small\n", stderr);
		return 1;
	}

	// probably MS-DOS file
	this->type = XT_DOS;

	struct dos *dos = this->blob.data;
	this->dos = dos;

	dbgf("dos start: %zX\n", start);

	size_t pe_start = dos->e_lfanew;

	if (pe_start >= size) {
		xtfprintf(stderr, "bad pe start: got %zX, max: %zX\n", start, size - 1);
		return 1;
	}

	dbgf("pe start: %zX\n", pe_start);

	if (size < sizeof(struct pehdr) + pe_start) {
		fputs("bad pe/coff header: file too small\n", stderr);
		return 1;
	}

	// probably winnt file
	this->type = XT_PE;
	this->bits = 32;

	struct pehdr *phdr = (struct pehdr*)((char*)this->blob.data + pe_start);

	this->pe = phdr;

	// verify fields
	if (phdr->f_magic != PE_MAGIC) {
		fprintf(stderr, "bad pe magic: got %X, expected %X\n", phdr->f_magic, PE_MAGIC);
		return 1;
	}

	if (!phdr->f_opthdr)
		return 0;

	if (size < sizeof(struct pehdr) + pe_start + sizeof(union peopthdr)) {
		fputs("bad pe/coff header: file too small\n", stderr);
		return 1;
	}

	// this is definitely a winnt file
	this->type = XT_PEOPT;

	const struct penthdr *nt = (struct penthdr*)(data + pe_start);

	union peopthdr *pohdr = (union peopthdr*)(data + pe_start + sizeof(struct pehdr));
	this->peopt = pohdr;

	dbgf("opthdr size: %hX\n", phdr->f_opthdr);

	switch (pohdr->hdr32.coff.o_magic) {
		case 0x10b:
			dbgs("type: portable executable 32 bit");
			this->bits = 32;
			this->nrvasz = pohdr->hdr32.o_nrvasz;
			break;
		case 0x20b:
			dbgs("type: portable executable 64 bit");
			this->bits = 64;
			this->nrvasz = pohdr->hdr64.o_nrvasz;
			break;
		default:
			dbgf("type: unknown: %I16X\n", pohdr->hdr32.coff.o_magic);
			this->bits = 0;
			return 1;
	}

	this->ddir = (union peddir*)(nt + penthdr_size(nt));

	// NOTE we ignore section markers
	size_t sec_start = pe_start + sizeof(struct pehdr) + phdr->f_opthdr;
	this->sec = (struct sechdr*)(data + sec_start);

	dbgf("sec_start: %zu\n", sec_start);
	return 0;
}

void pe_close(struct pe *pe)
{
	fs_blob_close(&pe->blob);
}

#define RSRC_DIR 0x80000000

#define DATA_OFFSET(data, ptr) ((uint64_t)((const char*)ptr - (const char*)data))

#define TD_NAMED 0x01
#define TD_DUMP_RES 0x02

static const char *rt_types[] = {
	"unknown",
	"cursor",
	"bitmap",
	"icon",
	"menu",
	"dialog",
	"string",
	"font directory",
	"font",
	"accelerator",
	"rc data",
	"message table",
	"group cursor",
	"group icon",
	// mysterious type 13 missing
	[RT_VERSION] = "version",
	"dialog include",
	// mysterious type 18 missing
	[RT_PLUGPLAY] = "plug and play",
	"vxd",
	"animated cursor",
	"animated icon",
};

int pe_load_res(struct pe *pe, unsigned type, unsigned id, void **ptr, size_t *count)
{
	const void *data = pe->blob.data;
	size_t size = pe->blob.size;
	unsigned language_id = 0; // TODO support multiple languages

	// ensure resource section is loaded
	if (!pe->rsrc) {
		const struct sechdr *sec = pe->sec;

		for (unsigned i = 0, n = pe->pe->f_nsect; i < n; ++i) {
			char name[9];
			memcpy(name, sec[i].s_name, sizeof name - 1);
			name[sizeof name - 1] = '\0';

			if (!strcmp(name, ".rsrc")) {
				dbgf("pe: load resource section (nr %u)\n", i);
				struct rsrctbl *rsrc = (struct rsrctbl*)((char*)data + sec[i].s_scnptr);
				if ((char*)&rsrc[1] > (char*)data + pe->blob.size) {
					dbgs("pe: bad rsrc: too small\n");
					return 1;
				}

				pe->rsrc = rsrc;
				pe->sec_rsrc = &sec[i];
				break;
			}
		}

		if (!pe->rsrc)
			return 1;
	}

	dbgf("load res: type=%u id=%u\n", type, id);

	struct rsrctbl *rsrc = pe->rsrc;
	time_t time = rsrc->r_time;

	const struct rsrcditem *item = (struct rsrcditem*)&rsrc[1];

	if (rsrc->r_nname) {
		dbgf("skipping %I32u named\n", rsrc->r_nname);
		item += rsrc->r_nname;
	}

	uint32_t t_id = rsrc->r_nid;

	for (uint32_t i = 0; i < rsrc->r_nid; ++i, ++item) {
		// traverse types (level one)
		if (item->r_id == type) {
			t_id = i;
			break;
		}
	}

	if (t_id == rsrc->r_nid) {
		dbgs("res not found");
		return 1;
	}

	if (!(item->r_rva & RSRC_DIR)) {
		dbgs("pe: unexpected type leaf");
		return 1;
	}

	uint32_t rva = item->r_rva & ~RSRC_DIR;

	// level two
	const struct rsrctbl *dirtbl = (struct rsrctbl*)((char*)rsrc + rva);
	dbgf("type: %s subdir: %I32X ids: %I16X\n", rt_types[item->r_id], rva, dirtbl->r_nid);

	// skip named items
	const struct rsrcditem *diritem = (struct rsrcditem*)&dirtbl[1];

	if (dirtbl->r_nname) {
		dbgf("skipping %I32u named dirs\n", dirtbl->r_nname);
		diritem += dirtbl->r_nname;
	}

	unsigned dirid = dirtbl->r_nid;

	for (unsigned i = 0; i < dirtbl->r_nid; ++i, ++diritem) {
		if (!(diritem->r_rva & RSRC_DIR)) {
			dbgs("pe: unexpected identifier leaf");
			return 1;
		}

		if (diritem->r_id == id || (type == RT_STRING && diritem->r_id - 1 == id / 16)) {
			dirid = i;
			break;
		}
	}

	if (dirid == dirtbl->r_nid) {
		dbgs("res not found");
		return 1;
	}

	rva = diritem->r_rva & ~RSRC_DIR;
	dbgf("resdir: id: %X rva: %X\n", diritem->r_id, rva);

	const struct rsrctbl *langtbl = (struct rsrctbl*)((char*)rsrc + rva);
	const struct rsrcditem *langitem = (struct rsrcditem*)&langtbl[1];

	unsigned langid = langtbl->r_nid;

	for (unsigned i = 0; i < langtbl->r_nid; ++i, ++langitem) {
		if (langitem->r_rva & RSRC_DIR) {
			dbgs("pe: unexpected language id dir");
			return 1;
		}

		if (!language_id || langitem->r_id == language_id) {
			langid = i;
			break;
		}
	}

	if (langid == langtbl->r_nid) {
		dbgs("res not found");
		return 1;
	}

	dbgf("langtbl: id: %X rva: %X\n", langitem->r_id, langitem->r_rva);

	const struct rsrcentry *entry = (struct rsrcentry*)((char*)rsrc + langitem->r_rva);

	off_t e_addr = pe->sec_rsrc->s_scnptr + entry->e_addr - pe->sec_rsrc->s_vaddr;

	dbgf("DataEntry: CodePage: %u vaddr: %X paddr: %X Size: %X\n",
		entry->e_cp, entry->e_addr, e_addr, entry->e_size);

	if (type != RT_STRING) {
		*ptr = (char*)data + e_addr;
		*count = entry->e_size;
		return 0;
	}

	uint16_t strid = (diritem->r_id - 1) * 16;

	const uint16_t *strptr = (uint16_t*)((char*)data + e_addr), *end = (uint16_t*)((char*)data + pe->blob.size - 2);

	for (unsigned i = 0; i < 16; ++i, ++strid) {
		const struct pestr *str = (struct pestr*)strptr;

		if (strptr > end || strptr + str->length > end) {
			fputs("pe: bad string table: unexpected EOF\n", stderr);
			return 1;
		}

		++strptr;

		if (strid == id) {
			dbgf("pe: found str: id=%u, size=%I16X\n", id, str->length * 2);
			*ptr = strptr;
			*count = str->length * 2;
			return 0;
		} else {
			strptr += str->length;
		}
	}

	dbgs("pe: res not found in strtbl");
	return 1;
}

int pe_load_string(struct pe *pe, unsigned id, char *str, size_t size)
{
	int err;
	void *ptr;
	size_t count;

	if (err = pe_load_res(pe, RT_STRING, id, &ptr, &count))
		return err;

	if (!size)
		return 0;

	count /= 2;
	size_t max = count >= size ? size - 1 : count;

	const uint16_t *data = ptr;

	for (size_t i = 0; i < count; ++i)
		str[i] = data[i];

	str[count] = '\0';

	dbgf("pe_load_string: id=%u: size=%zu\n", id, count);
	return 0;
}
