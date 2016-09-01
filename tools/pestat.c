#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct mz {
	uint16_t sig;
	uint16_t last;
	uint16_t npage;
	uint16_t nreloc;
	uint16_t hdrsz;
	uint16_t min, max;
	uint16_t ss, sp;
	uint16_t chksum;
	uint16_t ip, cs;
	uint16_t reloc;
	uint16_t overlay;
	uint8_t data[32];
	uint32_t e_lfanew;
} __attribute__((packed));

struct pehdr {
	uint32_t sig;
	uint16_t mach, nsect;
	uint32_t time;
	uint32_t symptr;
	uint32_t nsym;
	uint16_t optsz;
	uint16_t chr;
} __attribute__((packed));

struct peopt {
	uint16_t sig;
	uint8_t major, minor;
	uint32_t codesz, initsz, uninitsz;
	uint32_t entry, code, data;
	uint32_t image, section, file;
	uint16_t osmajor, osminor;
	uint16_t imgmajor, imgminor;
	uint16_t submajor, subminor;
	uint32_t win32;
	uint32_t imgsz, hdrsz;
	uint32_t chksum;
	uint16_t sub, dll;
	uint32_t stack_res, stack_commit;
	uint32_t heap_res, heap_commit;
	uint32_t loader, rva;
} __attribute__((packed));

struct rva {
	uint32_t exptbl, expsz;
	uint32_t intbl, insz;
	uint32_t restbl, ressz;
	uint32_t errtbl, errsz;
	uint32_t certtbl, certsz;
	uint32_t reloctbl, relocsz;
	uint32_t dbg, dbgsz;
	uint32_t copy, copysz;
	uint32_t global, g_pad;
	uint32_t tlstbl, tlssz;
	uint32_t lconftbl, lconfsz;
	uint32_t bound, boundsz;
	uint32_t inaddr, inaddrsz;
	uint32_t delaydesc, delaydescsz;
	uint32_t clrrun, clrrunsz;
	uint32_t res, res_pad;
};

struct sect {
	char name[8];
	union {
		uint32_t phys, virtsz;
	} misc;
	uint32_t virtaddr;
	uint32_t rawsz, rawaddr;
	uint32_t reloc, lno;
	uint16_t nreloc, nlno;
	uint32_t chr;
} __attribute__((packed));

#define SCHR_CODE 0x20
#define SCHR_INITDATA 0x40
#define SCHR_EXEC 0x20000000
#define SCHR_READ 0x40000000
#define SCHR_WRITE 0x80000000

void sect_dump(const struct sect *this) {
	char name[9];
	snprintf(name, sizeof name, "%s", this->name);
	printf("name: %s\n", name);
	printf("virtual size      : %X\n", this->misc.virtsz);
	printf("virtual address   : %X\n", this->virtaddr);
	printf("raw data size     : %X\n", this->rawsz);
	printf("raw data offset   : %X\n", this->rawaddr);
	printf("relocation offset : %X\n", this->reloc);
	printf("line number offset: %X\n", this->lno);
	printf("relocation count  : %X\n", this->nreloc);
	printf("line number count : %X\n", this->nlno);
	printf("characteristics   : %X //", this->chr);
	if (this->chr & SCHR_CODE)
		fputs(" CODE", stdout);
	if (this->chr & SCHR_INITDATA)
		fputs(" INITIALIZED_DATA", stdout);
	if (this->chr & SCHR_EXEC)
		fputs(" EXECUTE", stdout);
	if (this->chr & SCHR_READ)
		fputs(" READ", stdout);
	if (this->chr & SCHR_WRITE)
		fputs(" WRITE", stdout);
	putchar('\n');
}

void hexdump(const void *ptr, unsigned n) {
	unsigned i, j;
	const unsigned char *data = ptr;
	for (i = 0; i < n; i += 16) {
		printf("%08X ", (unsigned)i);
		for (j = i; j < i + 16 && j < n; ++j)
			printf("%02hhX ", data[j]);
		for (j = i; j < i + 16 && j < n; ++j)
			putchar(isprint(data[j]) ? data[j] : '.');
		putchar('\n');
	}
}

void rva_dump(const struct rva *this, size_t n) {
	printf("export table           : %X\n", this->exptbl);
	printf("export size            : %X\n", this->expsz);
	if (n < 8) return;
	printf("import table           : %X\n", this->intbl);
	printf("import size            : %X\n", this->insz);
	if (n < 16) return;
	printf("resource table         : %X\n", this->restbl);
	printf("resource size          : %X\n", this->ressz);
	if (n < 24) return;
	printf("exception table        : %X\n", this->errtbl);
	printf("exception size         : %X\n", this->errsz);
	if (n < 32) return;
	printf("certificate table      : %X\n", this->certtbl);
	printf("certificate size       : %X\n", this->certsz);
	if (n < 40) return;
	printf("relocation table       : %X\n", this->reloctbl);
	printf("relocation size        : %X\n", this->relocsz);
	if (n < 48) return;
	printf("debug table            : %X\n", this->dbg);
	printf("debug size             : %X\n", this->dbgsz);
	if (n < 56) return;
	printf("copyright table        : %X\n", this->copy);
	printf("copyright size         : %X\n", this->copysz);
	if (n < 64) return;
	printf("global pointer         : %X\n", this->global);
	printf("padding                : %X\n", this->g_pad);
	if (n < 72) return;
	printf("TLS table              : %X\n", this->tlstbl);
	printf("TLS size               : %X\n", this->tlssz);
	if (n < 80) return;
	printf("load config table      : %X\n", this->lconftbl);
	printf("load config size       : %X\n", this->lconfsz);
	if (n < 88) return;
	printf("bound import           : %X\n", this->bound);
	printf("bound size             : %X\n", this->boundsz);
	if (n < 96) return;
	printf("import address table   : %X\n", this->inaddr);
	printf("import address size    : %X\n", this->inaddrsz);
	if (n < 104) return;
	printf("delay import descriptor: %X\n", this->delaydesc);
	printf("delay import size      : %X\n", this->delaydescsz);
	if (n < 112) return;
	printf("CLR runtime header     : %X\n", this->clrrun);
	printf("CLR runtime size       : %X\n", this->clrrunsz);
	if (n < 120) return;
	printf("reserved               : %X\n", this->res);
	printf("reserved padding       : %X\n", this->res_pad);
	if (n < 128) return;
	fputs("too big\n", stderr);
}

void mz_dump(const struct mz *this) {
	printf("signature       : %hX\n", this->sig);
	printf("last page size  : %hu\n", this->last);
	printf("page count      : %hu\n", this->npage);
	printf("relocations     : %hu\n", this->nreloc);
	printf("header size     : %hu\n", this->hdrsz);
	printf("min paragraphs  : %hu\n", this->min);
	printf("max paragraphs  : %hu\n", this->max);
	printf("initial ss:sp   : %04hX:%04hX\n", this->ss, this->sp);
	printf("checksum        : %hX\n", this->chksum);
	printf("initial cs:ip   : %04hX:%04hX\n", this->ip, this->cs);
	printf("relocation table: %hX\n", this->reloc);
	printf("overlay         : %hu\n", this->overlay);
	printf("e_lfanew        : %hX\n", this->e_lfanew);
}

void pe_dumphdr(const struct pehdr *this) {
	printf("signature      : %X\n", this->sig);
	printf("machine        : %hX\n", this->mach);
	printf("section count  : %hX\n", this->nsect);
	printf("timedatestamp  : %X\n", this->time);
	printf("symbol table   : %X\n", this->symptr);
	printf("symbol count   : %X\n", this->nsym);
	printf("optional size  : %hX\n", this->optsz);
	printf("characteristics: %hX\n", this->chr);
}

void pe_dumpopt(const struct peopt *this) {
	printf("signature        : %hX\n", this->sig);
	printf("link major       : %hhX\n", this->major);
	printf("link minor       : %hhX\n", this->minor);
	printf("code size        : %X\n", this->codesz);
	printf("init size        : %X\n", this->initsz);
	printf("uninit size      : %X\n", this->uninitsz);
	printf("entry point      : %X\n", this->entry);
	printf("code offset      : %X\n", this->code);
	printf("data offset      : %X\n", this->data);
	printf("image alignment  : %X\n", this->image);
	printf("section alignment: %X\n", this->section);
	printf("file alignment   : %X\n", this->file);
	printf("OS major         : %hX\n", this->osmajor);
	printf("OS minor         : %hX\n", this->osminor);
	printf("image major      : %hX\n", this->imgmajor);
	printf("image minor      : %hX\n", this->imgminor);
	printf("subsys major     : %hX\n", this->submajor);
	printf("subsys minor     : %hX\n", this->subminor);
	printf("win32 version    : %X\n", this->win32);
	printf("image size       : %X\n", this->imgsz);
	printf("header size      : %X\n", this->hdrsz);
	printf("checksum         : %X\n", this->chksum);
	printf("subsystem        : %hX\n", this->sub);
	printf("dll flags        : %hX\n", this->dll);
	printf("stack reserve    : %X\n", this->stack_res);
	printf("stack commit     : %X\n", this->stack_commit);
	printf("heap reserve     : %X\n", this->heap_res);
	printf("heap commit      : %X\n", this->heap_commit);
	printf("loader flags     : %X\n", this->loader);
	printf("rva size/count   : %u\n", this->rva);
}

int main(int argc, char **argv) {
	int fd = -1, ret = 1;
	ssize_t zd;
	#define BUFSZ 4096
	char buf[BUFSZ];
	struct mz m;
	struct pehdr hdr;
	off_t off;
	printf("sizeof mz: %zu\n", sizeof(struct mz));
	printf("sizeof pehdr: %zu\n", sizeof(struct pehdr));
	printf("sizeof peopt: %zu\n", sizeof(struct peopt));
	printf("sizeof rva: %zu\n", sizeof(struct rva));
	printf("sizeof sect: %zu\n", sizeof(struct sect));
	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "pestat");
		return 1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror(argv[1]);
		goto fail;
	}
	zd = read(fd, buf, 0x40);
	if (zd != 0x40) {
		fputs("MZ header not found\n", stderr);
		goto fail;
	}
	memcpy(&m, buf, sizeof(struct mz));
	puts("MZ header:");
	mz_dump(&m);
	if ((off = lseek(fd, m.e_lfanew, SEEK_SET)) == (off_t)-1) {
		perror("seek");
		goto fail;
	}
	zd = read(fd, buf, sizeof(struct pehdr));
	if (zd != sizeof(struct pehdr)) {
		fputs("PE header not found\n", stderr);
		goto fail;
	}
	memcpy(&hdr, buf, sizeof(struct pehdr));
	puts("PE header:");
	pe_dumphdr(&hdr);
	if (hdr.optsz) {
		struct peopt opt;
		zd = read(fd, buf, sizeof opt);
		if (zd != sizeof opt) {
			fputs("PE opt corrupt\n", stderr);
			goto fail;
		}
		memcpy(&opt, buf, sizeof opt);
		puts("PE optional header:");
		pe_dumpopt(&opt);
		if (opt.rva) {
			struct rva r;
			size_t n = sizeof r;
			/* TODO verify this */
			if (opt.rva != 16) fputs("rva should be 16\n", stderr);
			zd = read(fd, &r, n);
			if (zd < 0 || (size_t)zd != n) {
				fputs("rva block corrupt\n", stderr);
				goto fail;
			}
			puts("RVA tables:");
			rva_dump(&r, opt.rva * sizeof(uint32_t));
		}
	}
{
	struct sect *s = NULL;
	size_t n = hdr.nsect * sizeof(struct sect);
	uint16_t i;
	s = malloc(n);
	if (!s) {
		perror("sect");
		goto fail;
	}
	zd = read(fd, s, n);
	if (zd < 0 || (size_t)zd != n) {
		perror("sect");
		goto fail;
	}
	printf("sect count: %hX\n", hdr.nsect);
	for (i = 0; i < hdr.nsect; ++i) {
		const struct sect *sect = &s[i];
		printf("sect %hX:\n", i);
		sect_dump(sect);
	}
	free(s);
}
	ret = 0;
fail:
	if (fd != -1) close(fd);
	return ret;
}
