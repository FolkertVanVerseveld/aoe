/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "pe.h"

#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#include <genie/dbg.h>

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

	char *data = this->blob.data;
	size_t size = this->blob.size;

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
		fprintf(stderr, "pe: bad dos magic: got %" PRIX16 ", %" PRIX16 " expected\n", mz->e_magic, DOS_MAGIC);
		return 1;
	}

	size_t start = mz->e_cparhdr * 16;

	if (start >= size) {
		fprintf(stderr, "pe: bad exe start: got " PRIX64 ", max: " PRIX64 "\n", (uint64_t)start, (uint64_t)(size - 1));
		return 1;
	}

	off_t data_start = mz->e_cp * 512;

	if (mz->e_cblp) {
		data_start -= 512 - mz->e_cblp;
		if (data_start < 0) {
			fprintf(stderr, "pe: bad data_start: -%" PRIX64 "\n", (uint64_t)-data_start);
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
		fprintf(stderr, "bad pe start: got %zX, max: %zX\n", start, size - 1);
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

	if (size < sizeof(struct pehdr) + pe_start + sizeof(struct peopthdr)) {
		fputs("bad pe/coff header: file too small\n", stderr);
		return 1;
	}

	// this is definitely a winnt file
	this->type = XT_PEOPT;
	struct peopthdr *pohdr = (struct peopthdr*)(data + pe_start + sizeof(struct pehdr));
	this->peopt = pohdr;

	dbgf("opthdr size: %hX\n", phdr->f_opthdr);

	switch (pohdr->o_chdr.o_magic) {
		case 0x10b:
			dbgs("type: portable executable 32 bit");
			break;
		case 0x20b:
			dbgs("type: portable executable 64 bit");
			this->bits = 64;
			break;
		default:
			dbgf("type: unknown: %" PRIX16 "\n", pohdr->o_chdr.o_magic);
			break;
	}

	this->nrvasz = pohdr->o_nrvasz;
	dbgf("nrvasz: %" PRIX32 "\n", pohdr->o_nrvasz);

	size_t sec_start = pe_start + sizeof(struct pehdr) + phdr->f_opthdr;

	// verifiy section parameters
	if (pohdr->o_ddir.d_end) {
		fprintf(stderr, "pe: bad data dir end marker: %lX\n", pohdr->o_ddir.d_end);

		// search for end marker
		while (++sec_start <= size - sizeof(uint64_t) && *((uint64_t*)(data + sec_start)))
			;

		if (sec_start > size - sizeof(uint64_t)) {
			fputs("pe: data dir end marker not found\n", stderr);
			return 1;
		}

		sec_start += sizeof(uint64_t);
	}

	if (sec_start + pohdr->o_nrvasz * sizeof(struct sechdr) > size) {
		fputs("pe: bad section table: file too small\n", stderr);
		return 1;
	}

#ifdef DEBUG
	struct sechdr *sec = this->sec = (struct sechdr*)(data + sec_start);

	for (unsigned i = 0; i < pohdr->o_nrvasz; ++i, ++sec) {
		char name[9];

		strncpy(name, sec->s_name, 9);
		name[8] = '\0';

		if (!name[0] && !sec->s_scnptr) {
			this->nrvan = i;
			break;
		}

		char buf[256];
		uint32_t flags = sec->s_flags;
		buf[0] = '\0';

		if (flags & SF_CODE) {
			flags &= ~SF_CODE;
			strcat(buf, " code");
		}
		if (flags & SF_DATA) {
			flags &= ~SF_DATA;
			strcat(buf, " data");
		}
		if (flags & SF_BSS) {
			flags &= ~SF_BSS;
			strcat(buf, " bss");
		}
		if (flags & SF_DBG) {
			flags &= ~SF_DBG;
			strcat(buf, " dbg");
		}
		if (flags & SF_LINK) {
			flags &= ~SF_LINK;
			strcat(buf, " link");
		}
		if (flags & SF_STRIP) {
			flags &= ~SF_STRIP;
			strcat(buf, " strip");
		}
		if (flags & SF_DIRECT) {
			flags &= ~SF_DIRECT;
			strcat(buf, " direct");
		}
		if (flags & SF_FIXED) {
			flags &= ~SF_FIXED;
			strcat(buf, " fixed");
		}
		if (flags & SF_SHARE) {
			flags &= ~SF_SHARE;
			strcat(buf, " share");
		}
		if (flags & SF_EXEC) {
			flags &= ~SF_EXEC;
			strcat(buf, " exec");
		}
		if (flags & SF_READ) {
			flags &= ~SF_READ;
			strcat(buf, " read");
		}
		if (flags & SF_WRITE) {
			flags &= ~SF_WRITE;
			strcat(buf, " write");
		}
		if (flags)
			strcat(buf, " ??");
		dbgf("#%2u: %-8s %8" PRIX32 " %8" PRIX32 "%s\n", i, name, sec->s_scnptr, sec->s_flags, buf);
	}
	dbgf("section count: %u (occupied: %u)\n", pohdr->o_nrvasz, this->nrvan);
	dbgf("text entry: %" PRIX64 "\n", (uint64_t)(pohdr->o_chdr.o_entry));
#endif

	return 0;
}

void pe_close(struct pe *pe)
{
	fs_blob_close(&pe->blob);
}
