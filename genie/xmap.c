/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "xmap.h"

#ifdef XMAPDBG
#define dbgs(s) puts(s)
#define fdbgs(s,f) fputs(s,f)
#define dbgf(f,...) printf(f, ##__VA_ARGS__)
#else
#define dbgs(s) ((void)0)
#define fdbgs(s,f) ((void)0)
#define dbgf(f,...) ((void)0)
#endif

int xstat(struct xfile *this, char *data, size_t size)
{
	this->type = XT_UNKNOWN;
	this->bits = 0;
	this->mz = NULL;
	this->dos = NULL;
	this->pe = NULL;
	this->peopt = NULL;
	this->nrvan = this->nrvasz = 0;
	this->sec = NULL;
	this->rsrc = NULL;
	this->data = data;
	this->size = size;
	if (size < sizeof(struct mz)) {
		fputs("bad dos header: file too small\n", stderr);
		return 1;
	}
	this->type = XT_MZ;
	this->bits = 16;
	struct mz *mz = (struct mz*)data;
	this->mz = mz;
	if (mz->e_magic != DOS_MAGIC)
		fprintf(stderr, "bad dos magic: got %hX, %hX expected\n", mz->e_magic, DOS_MAGIC);
	size_t start = mz->e_cparhdr * 16;
	if (start >= size) {
		fprintf(stderr, "bad exe start: got %zX, max: %zX\n", start, size - 1);
		return 1;
	}
	off_t data_start = mz->e_cp * 512;
	if (mz->e_cblp) {
		data_start -= 512 - mz->e_cblp;
		if (data_start < 0) {
			fprintf(stderr, "bad data_start: -%zX\n", -data_start);
			return 1;
		}
	}
	dbgf("data start: %zX\n", data_start);
	if (start < sizeof(struct dos)) {
		dbgf("dos start: %zX\n", start);
		return 0;
	}
	if (start >= size) {
		fputs("bad dos start: file too small\n", stderr);
		return 1;
	}
	this->type = XT_DOS;
	struct dos *dos = (struct dos*)data;
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
	this->bits = 32;
	this->type = XT_PE;
	struct pehdr *phdr = (struct pehdr*)(data + pe_start);
	this->pe = phdr;
	if (phdr->f_magic != PE_MAGIC)
		fprintf(stderr, "bad pe magic: got %X, expected %X\n", phdr->f_magic, PE_MAGIC);
	if (!phdr->f_opthdr)
		return 0;
	if (size < sizeof(struct pehdr) + pe_start + sizeof(struct peopthdr)) {
		fputs("bad pe/coff header: file too small\n", stderr);
		return 1;
	}
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
			dbgf("type: unknown: %hX\n", pohdr->o_chdr.o_magic);
			break;
	}
	this->nrvasz = pohdr->o_nrvasz;
	dbgf("nrvasz: %X\n", pohdr->o_nrvasz);
	size_t sec_start = pe_start + sizeof(struct pehdr) + phdr->f_opthdr;
	if (pohdr->o_ddir.d_end) {
		fprintf(stderr, "bad data dir end marker: %lX\n", pohdr->o_ddir.d_end);
		// search for end marker
		while (++sec_start <= size - sizeof(uint64_t) && *((uint64_t*)(data + sec_start)))
			;
		if (sec_start > size - sizeof(uint64_t)) {
			fputs("data dir end marker not found\n", stderr);
			return 1;
		}
		sec_start += sizeof(uint64_t);
	}
	if (sec_start + pohdr->o_nrvasz * sizeof(struct sechdr) > size) {
		fputs("bad section table: file too small\n", stderr);
		return 1;
	}
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
		dbgf("#%2u: %-8s %8X %8X%s\n", i, name, sec->s_scnptr, sec->s_flags, buf);
	}
	dbgf("section count: %u (occupied: %u)\n", pohdr->o_nrvasz, this->nrvan);
	dbgf("text entry: %zX\n", (size_t)(pohdr->o_chdr.o_entry));
	return 0;
}

int xmap(const char *name, int prot, int *f, char **m, size_t *msz)
{
	int ret = 1, fd = -1;
	struct stat st;
	char *map = MAP_FAILED;
	size_t mapsz;
	fd = open(name, O_RDONLY);
	if (fd == -1 || fstat(fd, &st) == -1) {
		perror(name);
		goto fail;
	}
	map = mmap(NULL, mapsz = st.st_size, prot, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		perror(name);
		goto fail;
	}
	*f = fd;
	*m = map;
	*msz = mapsz;
	ret = 0;
fail:
	if (ret) {
		if (map != MAP_FAILED)
			munmap(map, mapsz);
		if (fd != -1)
			close(fd);
	}
	return ret;
}

int xunmap(int fd, char *map, size_t mapsz)
{
	int ret = 0;
	if (map != MAP_FAILED) {
		ret = munmap(map, mapsz);
		if (ret) return ret;
	}
	if (fd != -1)
		ret = close(fd);
	return ret;
}
