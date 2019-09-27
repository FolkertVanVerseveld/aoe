/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "pe.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#include <genie/dbg.h>

#include <xt/string.h>

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

#define RSRC_NLEAF 16

#define HEAPSZ 65536

struct rsrcstr {
	const uint16_t length;
	const char str[];
};

struct rstrptr {
	unsigned id;
	struct rsrcstr *str;
};

static struct strheap {
	struct rstrptr a[HEAPSZ];
	unsigned n;
	unsigned str_id;
} strtbl;

#define parent(x) (((x)-1)/2)
#define right(x) (2*((x)+1))
#define left(x) (right(x)-1)

static inline void swap(struct strheap *h, unsigned a, unsigned b)
{
	struct rstrptr tmp;
	tmp = h->a[a];
	h->a[a] = h->a[b];
	h->a[b] = tmp;
}

static inline int rptrcmp(struct rstrptr *a, struct rstrptr *b)
{
	return a->id - b->id;
}

static inline void siftup(struct strheap *h, unsigned i)
{
	register unsigned j;
	register int cmp;
	while (i) {
		j = parent(i);
		if ((cmp = rptrcmp(&h->a[j], &h->a[i])) > 0)
			swap(h, j, i);
		else
			break;
		i = j;
	}
}

static inline void put(struct strheap *h, struct rstrptr *i)
{
	assert(h->n < HEAPSZ);
	h->a[h->n] = *i;
	siftup(h, h->n++);
}

#if 0
static void dump(const struct strheap *h)
{
	unsigned i;
	printf("heap len=%d\n", h->n);
	for (i = 0; i < h->n; ++i)
		printf(" %d", h->a[i].id);
	putchar('\n');
}
#endif

static int rsrc_strtbl(struct pe *x, unsigned level, off_t diff, size_t off)
{
	char *map = x->blob.data;
	size_t mapsz = x->blob.size;
	struct rsrcitem *i = (struct rsrcitem*)(map + off);

	if (off + sizeof(struct rsrcitem) > mapsz) {
		xtfprintf(stderr, "bad leaf at %zX: file too small\n", off);
		return 1;
	}
	if (diff < 0 && diff + (ssize_t)i->d_rva < 0) {
		xtfprintf(stderr, "bad leaf rva -%zX\n", (size_t)-diff);
		return 1;
	}
#if DEBUG
	xtprintf("%8zX ", off);
	for (unsigned l = 0; l < level; ++l)
		fputs("  ", stdout);
#endif

	size_t pp, p = i->d_rva + diff;
	pp = p;

	dbgf("strtbl at %zX: %zX bytes\n", p, (size_t)i->d_size);

	uint16_t j, k, n, w, *hw;
	char *str, buf[65536];

	for (k = 0; p < pp + i->d_size; ++k) {
		hw = (uint16_t*)(map + p);
		p += sizeof(uint16_t);
		w = *hw;

#if DEBUG
		xtprintf("%8zX ", p);
		for (unsigned l = 0; l < level; ++l)
			fputs("  ", stdout);
#endif

		if (w) {
			if (p + 2 * w > mapsz) {
				xtfprintf(stderr, "bad leaf at %zX: file too small\n", off);
				return 1;
			}
			dbgf("#%2u ", k);
			for (str = map + p, j = 0, n = w; j < n; str += 2)
				buf[j++] = *str;
			buf[j++] = '\0';
			dbgs(buf);

			struct rstrptr ptr;
			ptr.id = strtbl.str_id + k;
			ptr.str = (struct rsrcstr*)(map + p - 2);

			dbgf("rsrc heap: put (%u,%s) @%" PRIX64 "\n", ptr.id, buf, (uint64_t)p);
			put(&strtbl, &ptr);
		} else {
			printf("#%2u\n", k);
		}

		p += w * 2;
	}

	return 0;
}

static int rsrc_leaf(struct pe *x, unsigned level, size_t soff, off_t diff, size_t *off)
{
	char *map = x->blob.data;
	size_t mapsz = x->blob.size;

	if (*off + sizeof(struct rsrcditem) > mapsz) {
		xtfprintf(stderr, "bad leaf at %zX: file too small\n", *off);
		return 1;
	}

	struct rsrcditem *ri = (struct rsrcditem*)(map + *off);
	size_t loff = soff + ri->r_rva;

	if (rsrc_strtbl(x, level, diff, loff)) {
		xtfprintf(stderr, "corrupt strtbl at %zX\n", loff);
		return 1;
	}

	*off += sizeof(struct rsrcditem);

	return 0;
}

static int rsrc_walk(struct pe *x, unsigned level, size_t soff, off_t diff, size_t *off, unsigned tn_id, size_t *poff, size_t *sn)
{
	char *map = x->blob.data;
	size_t size = x->blob.size;

	dbgf("soff=%zu,off=%zu\n", soff, *off);

	if (*off + sizeof(struct rsrcdir) > size) {
		fprintf(stderr, "bad rsrc node at level %u: file too small\n", level);
		return 1;
	}
	if (level == 3) {
		fputs("bad rsrc: too many levels\n", stderr);
		return 1;
	}

	struct rsrcdir *d = (struct rsrcdir*)(map + *off);
#if DEBUG
	xtprintf("%8zX ", *off);
	for (unsigned l = 0; l < level; ++l)
		fputs("  ", stdout);
	printf(
		"ResDir Named:%02X ID:%02X TimeDate:%8X Vers:%u.%02u Char:%u\n",
		d->r_nnment, d->r_nident, d->r_timdat, d->r_major, d->r_minor, d->r_flags
	);
#endif

	unsigned n = d->r_nnment + d->r_nident;
	*off += sizeof(struct rsrcdir);

	if (*off + n * sizeof(struct rsrcdir) > size) {
		fprintf(stderr, "bad rsrc node at level %u: file too small\n", level);
		return 1;
	}

	int dir = 0;
	size_t poffp = *poff;

	for (unsigned i = 0; i < n; ++i) {
		struct rsrcditem *ri = (struct rsrcditem*)(map + *off);

		dbgf("rva item = %" PRIX64 "\n", (uint64_t)*off);

		if (dir) {
			if (rsrc_walk(x, level + 1, soff, diff, off, i < d->r_nnment ? TN_NAME : TN_ID, &poffp, sn))
				return 1;
			poffp += sizeof(struct rsrcditem);
		} else {
#if DEBUG
			xtprintf("%8zX ", *off);
			for (unsigned l = 0; l < level; ++l)
				fputs("  ", stdout);

			printf("id=%u,rva=%X,type=%s,poff=%zX,poffp=%zX,%u\n", ri->r_id, ri->r_rva, tn_id == TN_ID ? "id" : "name", *poff, poffp, level);
#endif

			// black magic
			if (level == 2) {
				if (*sn != 0) {
					if (poffp + sizeof(struct rsrcditem) >= size) {
						xtfprintf(stderr, "bad offset: %zX\n", poffp);
						return 1;
					}
					struct rsrcditem *m = (struct rsrcditem*)(map + poffp);
					strtbl.str_id = (m->r_id - 1) * 16;
					printf("resid=%u\n", strtbl.str_id);
				}
				++*sn;
			} else if (level == 1) {
				strtbl.str_id = (ri->r_id - 1) * 16;
				printf("resid=%u\n", strtbl.str_id);
			}

			poffp = *off + sizeof(struct rsrcditem);

			if ((ri->r_rva >> 31) & 1) {
				size_t roff = soff + (ri->r_rva & ~(1 << 31));

				if (rsrc_walk(x, level + 1, soff, diff, &roff, i < d->r_nnment ? TN_NAME : TN_ID, &poffp, sn))
					return 1;

				*off = roff;
				xtprintf("roff=%zX\n", roff);
				dir = 1;
			} else {
#if DEBUG
				printf("%8zX ", *off);
				for (unsigned l = 0; l < level; ++l)
					fputs("  ", stdout);

				printf("#%u ID: %8X Offset: %8X\n", i, ri->r_id, ri->r_rva);
#endif
				if (rsrc_leaf(x, level + 1, soff, diff, off))
					return 1;
			}
		}
	}
	return 0;
}

static int rsrc_stat(struct pe *x)
{
	struct sechdr *sec = x->sec;
	assert(sec);

	unsigned i;
	for (i = 0; i < x->peopt->o_nrvasz; ++i, ++sec) {
		char name[9];
		strncpy(name, sec->s_name, 9);
		name[8] = '\0';

		if (!strcmp(name, ".rsrc"))
			goto found;
	}
	fputs("pe: no .rsrc\n", stderr);
	return 1;
found:
	dbgf("goto %u@%zX\n", i, (size_t)sec->s_scnptr);

	size_t off = sec->s_scnptr, poff = off;
	size_t n = 0;
	off_t diff = (ssize_t)sec->s_scnptr - sec->s_vaddr;
	int ret = rsrc_walk(x, 0, sec->s_scnptr, diff, &off, 0, &poff, &n);

	dbgf("node count: %zu\n", n);
	return ret;
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
	dbgf("nrvasz: %I32X\n", pohdr->o_nrvasz);

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

	struct sechdr *sec = this->sec = (struct sechdr*)(data + sec_start);

	for (unsigned i = 0; i < pohdr->o_nrvasz; ++i, ++sec) {
		char name[9];

		strncpy(name, sec->s_name, 9);
		name[8] = '\0';

		if (!name[0] && !sec->s_scnptr) {
			this->nrvan = i;
			break;
		}

#ifdef DEBUG
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
#endif
	}
	dbgf("section count: %u (occupied: %u)\n", pohdr->o_nrvasz, this->nrvan);
	dbgf("text entry: %" PRIX64 "\n", (uint64_t)(pohdr->o_chdr.o_entry));

	return rsrc_stat(this);
}

void pe_close(struct pe *pe)
{
	fs_blob_close(&pe->blob);
}

int pe_load_string(unsigned id, char *str, size_t size)
{
	for (unsigned i = 0; i < strtbl.n; ++i)
		if (strtbl.a[i].id == id) {
			dbgf("pe: load str %u\n", id);

			const struct rsrcstr *text = strtbl.a[i].str;
			size_t copy = size;
			if (copy > text->length)
				copy = text->length;

			// FIXME convert UCS2 to UTF8
			// just truncate all UCS2 chars for now
			for (size_t j = 0; j < copy; ++j)
				str[j] = text->str[2 * j];

			if (size) {
				if (copy < size)
					str[copy] = '\0';
				str[size - 1] = '\0';
			}

			return 0;
		}

	return 1;
}
