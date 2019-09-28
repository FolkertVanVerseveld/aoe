#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

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

// NO typedef bullshit like windoze uses ad nauseam!
// also use our own names for our own sanity

// IMAGE DOS HEADER
struct dos {
	uint16_t e_magic;
	uint16_t e_cblp;
	uint16_t e_cp;
	uint16_t e_crlc;
	uint16_t e_cparhdr;
	uint16_t e_minalloc;
	uint16_t e_maxalloc;
	uint16_t e_ss;
	uint16_t e_sp;
	uint16_t e_csum;
	uint16_t e_ip;
	uint16_t e_cs;
	uint16_t e_lfarlc;
	uint16_t e_ovno;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint32_t e_lfanew;
};

// IMAGE FILE HEADER
struct pehdr {
	uint16_t Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
};

// IMAGE OPTIONAL HEADER
struct peopthdr32 {
	uint16_t Magic;
	uint8_t MajorLinkerVersion;
	uint8_t MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint32_t BaseOfData;
	uint32_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;
	uint16_t DllCharacteristics;
	uint32_t SizeOfStackReserve;
	uint32_t SizeOfStackCommit;
	uint32_t SizeOfHeapCommit;
	uint32_t SizeOfHeapReserve;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
};

// IMAGE OPTIONAL HEADER PE32+
struct peopthdr64 {
	uint16_t Magic;
	uint8_t MajorLinkerVersion;
	uint8_t MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint64_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;
	uint16_t DllCharacteristics;
	uint64_t SizeOfStackReserve;
	uint64_t SizeOfStackCommit;
	uint64_t SizeOfHeapCommit;
	uint64_t SizeOfHeapReserve;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
};

// IMAGE OPTIONAL HEADER
// you see, microsoft has a great naming scheme... this optional header isn't optional
union peopthdr {
	struct peopthdr32 hdr32;
	struct peopthdr64 hdr64;
};

struct peditem {
	uint32_t VirtualAddress;
	uint32_t Size;
};

// from xmap.h struct peddir
struct peddef {
	uint32_t d_texp;
	uint32_t d_nexp;
	uint32_t d_timp;
	uint32_t d_nimp;
	uint32_t d_tres;
	uint32_t d_nres;
	uint32_t d_texc;
	uint32_t d_nexc;
	uint32_t d_tcrt;
	uint32_t d_ncrt;
	uint32_t d_trel;
	uint32_t d_nrel;
	uint32_t d_dbg;
	uint32_t d_ndbg;
	uint32_t d_arch;
	uint32_t d_narch;
	uint32_t d_glbl;
	uint32_t d_gzero;
	uint32_t d_ttls;
	uint32_t d_ntls;
	uint32_t d_tcfg;
	uint32_t d_ncfg;
	uint32_t d_bimp;
	uint32_t d_nbimp;
	uint32_t d_tiaddr;
	uint32_t d_niaddr;
	uint32_t d_did;
	uint32_t d_ndid;
	uint32_t d_crth;
	uint32_t d_ncrth;
	uint64_t d_end;
};

union peddir {
	struct peditem items[16];
	struct peddef data;
};

#define SF_CODE   0x00000020
#define SF_DATA   0x00000040
#define SF_BSS    0x00000080
#define SF_DBG    0x00000200
#define SF_LINK   0x00000800
#define SF_STRIP  0x02000000
#define SF_DIRECT 0x04000000
#define SF_FIXED  0x08000000
#define SF_SHARE  0x10000000
#define SF_EXEC   0x20000000
#define SF_READ   0x40000000
#define SF_WRITE  0x80000000

struct sechdr {
	char s_name[8]; // Name
	uint32_t s_paddr; // PhysicalAddress/VirtualSize
	uint32_t s_vaddr; // VirtualAddress
	uint32_t s_size; // SizeOfRawData
	uint32_t s_scnptr; // PointerToRawData
	uint32_t s_relptr; // PointerToRelocations
	uint32_t s_lnnoptr; // PointerToLineNumbers
	uint16_t s_nreloc; // NumberOfRelocations
	uint16_t s_nlnno; // NumberOfLineNumbers
	uint32_t s_flags; // Characteristics
};

struct rsrctbl {
	uint32_t r_flags; // Characteristics. Must be 0
	uint32_t r_time; // Time/Date Stamp
	uint16_t r_major; // Major Version
	uint16_t r_minor; // Minor Version
	uint16_t r_nname; // Number of Name Entries
	uint16_t r_nid; // Number of ID Entries
};

struct rsrcditem {
	uint32_t r_id;
	uint32_t r_rva;
};

struct rsrcentry {
	uint32_t e_addr; // OffsetToData
	uint32_t e_size; // Size
	uint32_t e_cp; // CodePage
	uint32_t e_res; // Reserved
};

// IMAGE NT HEADERS
struct penthdr {
	uint32_t Signature;
	struct pehdr FileHeader;
	union peopthdr OptionalHeader;
};

static inline size_t peopthdr_size(const union peopthdr *opt)
{
	return opt->hdr32.Magic == 0x10b ? sizeof(struct peopthdr32) : sizeof(struct peopthdr64);
}

static inline uint32_t peopthdr_rva_count(const union peopthdr *opt)
{
	return opt->hdr32.Magic == 0x10b ? opt->hdr32.NumberOfRvaAndSizes : opt->hdr64.NumberOfRvaAndSizes;
}

static inline size_t penthdr_size(const struct penthdr *nt)
{
	return sizeof *nt - sizeof(union peopthdr) + peopthdr_size(&nt->OptionalHeader);
}

void dump_dos_header(const struct dos *dos)
{
	puts("=== MZ Header ===");
	puts("");
	printf("signature               : %4" PRIX16 "\n", dos->e_magic);
	printf("bytes in last block     : %4" PRIX16 "\n", dos->e_cblp);
	printf("blocks in file          : %4" PRIX16 "\n", dos->e_cp);
	printf("relocation count        : %4" PRIX16 "\n", dos->e_crlc);
	printf("paragraph headers       : %4" PRIX16 "\n", dos->e_cparhdr);
	printf("min extra paragraphs    : %4" PRIX16 "\n", dos->e_minalloc);
	printf("max extra paragraphs    : %4" PRIX16 "\n", dos->e_maxalloc);
	printf("initial stack segment   : %4" PRIX16 "\n", dos->e_ss);
	printf("initial stack pointer   : %4" PRIX16 "\n", dos->e_sp);
	printf("checksum                : %4" PRIX16 "\n", dos->e_csum);
	printf("initial program counter : %4" PRIX16 "\n", dos->e_ip);
	printf("initial code segment    : %4" PRIX16 "\n", dos->e_cs);
	printf("relocatable table offset: %4" PRIX16 "\n", dos->e_lfarlc);
	printf("overlay number          : %4" PRIX16 "\n", dos->e_ovno);
	printf("reserved                : %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 "\n", dos->e_res[0], dos->e_res[1], dos->e_res[2], dos->e_res[3]);
	printf("OEM identifier          : %04" PRIX16 "\n", dos->e_oemid);
	printf("OEM information         : %04" PRIX16 "\n", dos->e_oeminfo);
	printf("reserved2               : %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16 "\n",
		dos->e_res2[0], dos->e_res2[1], dos->e_res2[2], dos->e_res2[3], dos->e_res2[4], dos->e_res2[5], dos->e_res2[6], dos->e_res2[7], dos->e_res2[8], dos->e_res2[9]);
	printf("lfanew                  : %" PRIX32 "\n", dos->e_lfanew);
}

void dump_file_header(const struct pehdr *file)
{
	time_t time = file->TimeDateStamp;
	puts("# IMAGE FILE HEADER");
	printf("Machine             :     %04" PRIX16 "\n", file->Machine);
	printf("NumberOfSections    :     %4" PRIX16 "\n", file->NumberOfSections);
	printf("TimeDateStamp       : %s", ctime(&time));
	printf("PointerToSymbolTable: %8" PRIX32 "\n", file->PointerToSymbolTable);
	printf("NumberOfSymbols     : %8" PRIX32 "\n", file->NumberOfSymbols);
	printf("SizeOfOptionalHeader:     %4" PRIX16 "\n", file->SizeOfOptionalHeader);
	printf("Characteristics     :     %04" PRIX16 "\n", file->Characteristics);
}

void dump_optional_header32(const struct peopthdr32 *opt)
{
	puts("# IMAGE OPTIONAL HEADER32");
	printf("Magic                      :     %04" PRIX16 "\n", opt->Magic);
	printf("MajorLinkerVersion         :       %02" PRIX8 "\n", opt->MajorLinkerVersion);
	printf("MinorLinkerVersion         :       %02" PRIX8 "\n", opt->MinorLinkerVersion);
	printf("SizeOfCode                 : %8" PRIX32 "\n", opt->SizeOfCode);
	printf("SizeOfInitializedData      : %8" PRIX32 "\n", opt->SizeOfInitializedData);
	printf("SizeOfUninitializedData    : %8" PRIX32 "\n", opt->SizeOfUninitializedData);
	printf("AddressOfEntryPoint        : %8" PRIX32 "\n", opt->AddressOfEntryPoint);
	printf("BaseOfCode                 : %8" PRIX32 "\n", opt->BaseOfCode);
	printf("BaseOfData                 : %8" PRIX32 "\n", opt->BaseOfData);
	printf("ImageBase                  : %8" PRIX32 "\n", opt->ImageBase);
	printf("SectionAlignment           : %8" PRIX32 "\n", opt->SectionAlignment);
	printf("FileAlignment              : %8" PRIX32 "\n", opt->FileAlignment);
	printf("MajorOperatingSystemVersion: %8" PRIX16 "\n", opt->MajorOperatingSystemVersion);
	printf("MinorOperatingSystemVersion: %8" PRIX16 "\n", opt->MinorOperatingSystemVersion);
	printf("MajorImageVersion          : %8" PRIX16 "\n", opt->MajorImageVersion);
	printf("MinorImageVersion          : %8" PRIX16 "\n", opt->MinorImageVersion);
	printf("MajorSubsystemVersion      : %8" PRIX16 "\n", opt->MajorSubsystemVersion);
	printf("MinorSubsystemVersion      : %8" PRIX16 "\n", opt->MinorSubsystemVersion);
	printf("Win32VersionValue          : %8" PRIX32 "\n", opt->Win32VersionValue);
	printf("SizeOfImage                : %8" PRIX32 "\n", opt->SizeOfImage);
	printf("SizeOfHeaders              : %8" PRIX32 "\n", opt->SizeOfHeaders);
	printf("CheckSum                   : %8" PRIX32 "\n", opt->CheckSum);
	printf("Subsystem                  :     %4" PRIX16 "\n", opt->Subsystem);
	printf("DllCharacteristics         :     %04" PRIX16 "\n", opt->DllCharacteristics);
	printf("SizeOfStackReserve         : %8" PRIX32 "\n", opt->SizeOfStackReserve);
	printf("SizeOfStackCommit          : %8" PRIX32 "\n", opt->SizeOfStackCommit);
	printf("SizeOfHeapCommit           : %8" PRIX32 "\n", opt->SizeOfHeapCommit);
	printf("SizeOfHeapReserve          : %8" PRIX32 "\n", opt->SizeOfHeapReserve);
	printf("LoaderFlags                : %8" PRIX32 "\n", opt->LoaderFlags);
	printf("NumberOfRvaAndSizes        : %8" PRIX32 "\n", opt->NumberOfRvaAndSizes);
}

void dump_optional_header64(const struct peopthdr64 *opt)
{
	puts("# IMAGE OPTIONAL HEADER64");
	printf("Magic                      :             %04" PRIX16 "\n", opt->Magic);
	printf("MajorLinkerVersion         :               %02" PRIX8 "\n", opt->MajorLinkerVersion);
	printf("MinorLinkerVersion         :               %02" PRIX8 "\n", opt->MinorLinkerVersion);
	printf("SizeOfCode                 :         %8" PRIX32 "\n", opt->SizeOfCode);
	printf("SizeOfInitializedData      :         %8" PRIX32 "\n", opt->SizeOfInitializedData);
	printf("SizeOfUninitializedData    :         %8" PRIX32 "\n", opt->SizeOfUninitializedData);
	printf("AddressOfEntryPoint        :         %8" PRIX32 "\n", opt->AddressOfEntryPoint);
	printf("BaseOfCode                 :         %8" PRIX32 "\n", opt->BaseOfCode);
	printf("ImageBase                  : %16" PRIX64 "\n", opt->ImageBase);
	printf("SectionAlignment           :         %8" PRIX32 "\n", opt->SectionAlignment);
	printf("FileAlignment              :         %8" PRIX32 "\n", opt->FileAlignment);
	printf("MajorOperatingSystemVersion:         %8" PRIX16 "\n", opt->MajorOperatingSystemVersion);
	printf("MinorOperatingSystemVersion:         %8" PRIX16 "\n", opt->MinorOperatingSystemVersion);
	printf("MajorImageVersion          :         %8" PRIX16 "\n", opt->MajorImageVersion);
	printf("MinorImageVersion          :         %8" PRIX16 "\n", opt->MinorImageVersion);
	printf("MajorSubsystemVersion      :         %8" PRIX16 "\n", opt->MajorSubsystemVersion);
	printf("MinorSubsystemVersion      :         %8" PRIX16 "\n", opt->MinorSubsystemVersion);
	printf("Win32VersionValue          :         %8" PRIX32 "\n", opt->Win32VersionValue);
	printf("SizeOfImage                :         %8" PRIX32 "\n", opt->SizeOfImage);
	printf("SizeOfHeaders              :         %8" PRIX32 "\n", opt->SizeOfHeaders);
	printf("CheckSum                   :         %8" PRIX32 "\n", opt->CheckSum);
	printf("Subsystem                  :             %4" PRIX16 "\n", opt->Subsystem);
	printf("DllCharacteristics         :             %04" PRIX16 "\n", opt->DllCharacteristics);
	printf("SizeOfStackReserve         :         %8" PRIX64 "\n", opt->SizeOfStackReserve);
	printf("SizeOfStackCommit          :         %8" PRIX64 "\n", opt->SizeOfStackCommit);
	printf("SizeOfHeapCommit           :         %8" PRIX64 "\n", opt->SizeOfHeapCommit);
	printf("SizeOfHeapReserve          :         %8" PRIX64 "\n", opt->SizeOfHeapReserve);
	printf("LoaderFlags                :         %8" PRIX32 "\n", opt->LoaderFlags);
	printf("NumberOfRvaAndSizes        :         %8" PRIX32 "\n", opt->NumberOfRvaAndSizes);
}

void dump_optional_header(const union peopthdr *opt)
{
	if (opt->hdr32.Magic == 0x10b)
		dump_optional_header32(&opt->hdr32);
	else
		dump_optional_header64(&opt->hdr64);
}

void dump_nt_headers(const struct penthdr *nt)
{
	puts("=== PE HEADER ===");
	puts("");
	printf("Signature: %08" PRIX32 "\n", nt->Signature);

	puts("");
	dump_file_header(&nt->FileHeader);
	puts("");
	dump_optional_header(&nt->OptionalHeader);
}

void dump_data_dir(const union peddir *dd, size_t count)
{
	if (count != 16)
		fprintf(stderr, "dump_data_dir: count should be 0x10, got: %" PRIX64 "\n", (uint64_t)count);

	puts("=== DATA DIRECTORY ===");
	puts("");

	const char *names[15] = {
		"EXPORT",
		"IMPORT",
		"RESOURCE",
		"EXCEPTION",
		"SECURITY",
		"BASERELOC",
		"DEBUG",
		"ARCHITECTURE",
		"GLOBALPTR",
		"TLS",
		"LOAD_CONFIG",
		"Bound_IAT",
		"IAT",
		"Delay_IAT",
		"CLR_Header",
	};

	for (unsigned i = 0; i < count; ++i)
		printf("%-12s RVA: %8" PRIX32 "  Size: %8" PRIX32 "\n",
			i < ARRAY_SIZE(names) ? names[i] : "",
			dd->items[i].VirtualAddress, dd->items[i].Size);
}

void dump_sechdr(const struct sechdr *hdr, size_t count)
{
	puts("=== SECTIONS ===\n\nNAME     VSZ/Phys      RVA  RawSize   RawPtr   RelPtr PtrReloc PtrLn   Lno    Flags");

	for (size_t i = 0; i < count; ++i) {
		char sf[4];
		char name[9];

		memcpy(name, hdr[i].s_name, sizeof name - 1);
		name[sizeof name - 1] = '\0';

		uint32_t flags = hdr[i].s_flags;

		sf[0] = flags & SF_READ ? 'R' : '-';
		sf[1] = flags & SF_WRITE ? 'W' : '-';
		sf[2] = flags & SF_EXEC ? 'X' : '-';
		sf[3] = '\0';

		printf("%-8s %8" PRIX32 " %8" PRIX32 " %8" PRIX32
			" %8" PRIX32 " %8" PRIX32 " %8" PRIX32
			"  %4" PRIX16 "  %4" PRIX16 " %8" PRIX32 " %s",
			name, hdr[i].s_paddr, hdr[i].s_vaddr, hdr[i].s_size,
			hdr[i].s_scnptr, hdr[i].s_relptr, hdr[i].s_lnnoptr,
			hdr[i].s_nreloc, hdr[i].s_nlnno, hdr[i].s_flags, sf);

		if (flags & SF_CODE) fputs(" CODE", stdout);
		if (flags & SF_DATA) fputs(" DATA", stdout);
		if (flags & SF_BSS) fputs(" BSS", stdout);
		if (flags & SF_DBG) fputs(" DBG", stdout);
		if (flags & SF_LINK) fputs(" LINK", stdout);
		if (flags & SF_STRIP) fputs(" STRIP", stdout);
		if (flags & SF_DIRECT) fputs(" DIRECT", stdout);
		if (flags & SF_FIXED) fputs(" FIXED", stdout);
		if (flags & SF_SHARE) fputs(" SHARE", stdout);

		putchar('\n');
	}
}

#define RSRC_DIR 0x80000000

#define DATA_OFFSET(data, ptr) ((uint64_t)((const char*)ptr - (const char*)data))

// Only define resource types if we are not on windoze
#ifndef _WIN32
#define RT_UNKNOWN      0
#define RT_CURSOR       1
#define RT_BITMAP       2
#define RT_ICON         3
#define RT_MENU         4
#define RT_DIALOG       5
#define RT_STRING       6
#define RT_FONTDIR      7
#define RT_FONT         8
#define RT_ACCELERATOR  9
#define RT_RCDATA       10
#define RT_MESSAGETABLE 11
#define RT_GROUP_CURSOR 12
#define RT_GROUP_ICON   14
#define RT_VERSION      16
#define RT_DLGINCLUDE   17
#define RT_PLUGPLAY     19
#define RT_VXD          20
#define RT_ANICURSOR    21
#define RT_ANIICON      22
#endif

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

int dump_langdir(const struct sechdr *sec, const struct rsrctbl *root, const struct rsrctbl *typedir, const void *data, size_t size, uint32_t count, uint32_t type)
{
	const char *end = (char*)data + size;

	if ((const char*)&typedir[1] + count * sizeof(struct rsrcditem) >= end) {
		fputs("bad langdir: unexpected EOF\n", stderr);
		return 1;
	}

	const struct rsrcditem *items = (struct rsrcditem*)&typedir[1];

	for (uint32_t i = 0; i < count; ++i) {
		if (!(items[i].r_rva & RSRC_DIR)) {
			fprintf(stderr, "bad langdir: not a subdir at %" PRIX64 "\n", DATA_OFFSET(data, &items[i]));
			return 1;
		}
		printf("%8" PRIX64 ":             ResDir: id: %" PRIX32 " SubDir: %" PRIX32 "\n", DATA_OFFSET(data, &items[i]), items[i].r_id, items[i].r_rva & ~RSRC_DIR);

		const struct rsrctbl *langdir = (struct rsrctbl*)((char*)root + (items[i].r_rva & ~RSRC_DIR));

		if ((char*)&langdir[1] >= end) {
			fprintf(stderr, "bad langdir: invalid subdir location %" PRIX32 "\n", items[i].r_rva & ~RSRC_DIR);
			return 1;
		}

		time_t time = langdir->r_time;

		printf("%8" PRIX64 ":                 ResTbl Flags: %" PRIX32 " Named: %" PRIX32 " IDs: %" PRIX32 " v%" PRIX16 ".%" PRIX16 " %s",
			DATA_OFFSET(data, langdir), langdir->r_flags, langdir->r_nname, langdir->r_nid, langdir->r_major, langdir->r_minor, ctime(&time));

		// NOTE we assume that langdir->r_nid == 1 && langdir->r_nname == 0

		const struct rsrcditem *leaf = (struct rsrcditem*)&langdir[1];

		if ((char*)&leaf[1] >= end) {
			fputs("bad resource leaf: unexpected EOF\n", stderr);
			return 1;
		}

		// parse language item
		printf("%8" PRIX64 ":                     ResLang: ID: %" PRIX32 " SubDir: %" PRIX32 "\n",
			DATA_OFFSET(data, leaf), leaf->r_id, leaf->r_rva);

		const struct rsrcentry *entry = (struct rsrcentry*)((char*)root + leaf->r_rva);

		if ((char*)&entry[1] >= end) {
			fputs("bad resource entry: unexpected EOF\n", stderr);
			return 1;
		}

		// real address: .rsrc_start + res.rva - image_base.rva
		off_t e_addr = sec->s_scnptr + entry->e_addr - sec->s_vaddr;

		printf("%8" PRIX64 ":                         DataEntry: CodePage: %" PRIX32 " vaddr: %" PRIX32 " paddr: %" PRIX64 " Size: %" PRIX32 "\n",
			DATA_OFFSET(data, entry), entry->e_cp, entry->e_addr, e_addr, entry->e_size);

		if ((char*)data + e_addr >= end) {
			fprintf(stderr, "bad resource entry: address: %" PRIX64 ", max %" PRIX64 "\n",
				e_addr, size);
			return 1;
		}

		switch (type) {
		case RT_STRING: {
			/*
			 * String tables always have 16 items.
			 * The resources indices are implicit and based on the parent node * 16
			 */
			uint16_t strid = items[i].r_id * 16;

			// dump the string name
			char buf[1024];
			// NOTE end points to last valid position
			const uint16_t *ptr = (uint16_t*)((char*)data + e_addr), *end = (uint16_t*)((char*)data + size - 2);

			for (unsigned i = 0; i < 16; ++i, ++strid) {
				const struct pestr *str = (const struct pestr*)ptr;

				if (ptr > end || ptr + str->length > end) {
					fputs("bad string table: unexpected EOF\n", stderr);
					return 1;
				}

				unsigned max = str->length >= sizeof buf ? sizeof buf - 1 : str->length;

				++ptr;

				for (unsigned j = 0; j < max; ++j)
					buf[j] = ptr[j];

				ptr += str->length;
				buf[max] = '\0';

				if (str->length)
					printf("%8" PRIX64 ": %" PRIu16 ": Size: %" PRIu16 " Data: \"%s\"\n", DATA_OFFSET(data, str), strid, str->length, buf);
			}
			break;
		}
		}
	}

	return 0;
}

int dump_typedir(const struct sechdr *sec, const struct rsrctbl *root, const struct rsrcditem *item, const void *data, size_t size, int named)
{
	const char *end = (char*)data + size;

	if ((const char*)&item[1] >= end) {
		fputs("bad typedir: unexpected EOF\n", stderr);
		return 1;
	}

	if (!(item->r_rva & RSRC_DIR)) {
		fputs("bad typedir: unexpected leaf\n", stderr);
		return 1;
	}

	uint32_t r_id = item->r_id;

	if (r_id < ARRAY_SIZE(rt_types)) {
		printf("%8" PRIX64 ":     ResDir Type: %s SubDir: %" PRIX32 "\n",
			DATA_OFFSET(data, item), rt_types[r_id], item->r_rva & ~RSRC_DIR);
	} else {
		if (named && (r_id & RSRC_DIR)) {
			r_id &= ~RSRC_DIR;

			const struct pestr *str = (struct pestr*)((char*)root + r_id);

			if ((char*)&str[1] >= end || (char*)root + str->length >= end) {
				fputs("bad named typedir: unexpected EOF\n", stderr);
				return 1;
			}

			char buf[256];
			uint16_t count = str->length >= sizeof buf ? sizeof buf - 1 : str->length;

			for (uint16_t i = 0; i < count; ++i)
				buf[i] = str->data[2 * i];

			buf[count] = '\0';

			printf("%8" PRIX64 ":     ResDir Name: %s SubDir: %" PRIX32 "\n",
				DATA_OFFSET(data, item), buf, item->r_rva & ~RSRC_DIR);
		} else {
			printf("%8" PRIX64 ":     ResDir Type: %" PRIX32 " SubDir: %" PRIX32 "\n",
				DATA_OFFSET(data, item), r_id, item->r_rva & ~RSRC_DIR);
		}
	}

	const struct rsrctbl *typedir = (struct rsrctbl*)((char*)root + (item->r_rva & ~RSRC_DIR));

	if ((const char*)&typedir[1] >= end) {
		fputs("bad typedir subdir: unexpected EOF\n", stderr);
		return 1;
	}

	time_t time = typedir->r_time;

	printf("%8" PRIX64 ":         ResTbl Flags: %" PRIX32 " Named: %" PRIX32 " IDs: %" PRIX32 " v%" PRIX16 ".%" PRIX16 " %s",
		DATA_OFFSET(data, typedir), typedir->r_flags, typedir->r_nname, typedir->r_nid, typedir->r_major, typedir->r_minor, ctime(&time));

	switch (r_id) {
	case RT_UNKNOWN:
	case RT_CURSOR:
	case RT_BITMAP:
	case RT_ICON:
	case RT_MENU:
	case RT_DIALOG:
	case RT_STRING:
	case RT_FONTDIR:
	case RT_FONT:
	case RT_ACCELERATOR:
	case RT_RCDATA:
	case RT_MESSAGETABLE:
	case RT_GROUP_CURSOR:
	case RT_GROUP_ICON:
	case RT_VERSION:
	case RT_DLGINCLUDE:
	case RT_PLUGPLAY:
	case RT_VXD:
	case RT_ANICURSOR:
	case RT_ANIICON:
		return dump_langdir(sec, root, typedir, data, size, typedir->r_nid, item->r_id);
	default:
		if (named) {
			fputs("TODO: parse named resource type\n", stderr);
			return 0;
		} else {
			fprintf(stderr, "%8" PRIX64 ": Unknown resource type: %" PRIX32 "\n", DATA_OFFSET(data, item), item->r_id);
			return 1;
		}
	}

	return 0;
}

void dump_rsrc(const struct sechdr *hdr, const void *data, size_t size)
{
	const char *end = (char*)data + size;
	const struct rsrctbl *tbl = (const struct rsrctbl*)((char*)data + hdr->s_scnptr);

	if ((const char*)&tbl[1] >= end) {
		fputs("bad rsrc: too small\n", stderr);
		return;
	}

	puts("=== RESOURCE TABLE ===\n");

	time_t time = tbl->r_time;

	// We should have used %16 for raw offset, but it is very rare to see PE's larger than 4GB.
	printf("%8" PRIX64 ": ResTbl flags: %" PRIX32 " named: %" PRIX32 " IDs: %" PRIX32 " v%" PRIX16 ".%" PRIX16 " %s",
		DATA_OFFSET(data, tbl), tbl->r_flags, tbl->r_nname, tbl->r_nid, tbl->r_major, tbl->r_minor, ctime(&time));

	const struct rsrcditem *item = (const struct rsrcditem*)&tbl[1];

	// FIXME support named items: setupenu.dll has them
	uint32_t j = 0;

	for (uint32_t i = 0; i < tbl->r_nname; ++i, ++j)
		if (dump_typedir(hdr, tbl, &item[j], data, size, 1))
			break;

	for (uint32_t i = 0; i < tbl->r_nid; ++i, ++j)
		if (dump_typedir(hdr, tbl, &item[j], data, size, 0))
			break;
}

int dump_pe(const void *data, size_t size)
{
	if (size < sizeof(struct dos)) {
		fputs("bad PE: too small\n", stderr);
		return 1;
	}

	dump_dos_header(data);

	const struct dos *dos = data;
	const struct penthdr *nt = (const struct penthdr*)((char*)data + dos->e_lfanew);
	const char *end = (const char*)data + size;

	if ((const char*)&nt[1] >= end) {
		fputs("bad nt header address\n", stderr);
		return 1;
	}

	puts("");
	dump_nt_headers(nt);

	const union peddir *dir = (const union peddir*)((char*)nt + penthdr_size(nt));

	if ((const char*)&dir[1] >= end) {
		fputs("bad data dir address\n", stderr);
		return 1;
	}

	puts("");
	dump_data_dir(dir, peopthdr_rva_count(&nt->OptionalHeader));

	const struct sechdr *sec = (const struct sechdr*)&dir[1];
	size_t nsec = nt->FileHeader.NumberOfSections;

	if ((const char*)&sec[nsec] >= end) {
		fputs("bad section count\n", stderr);
		return 1;
	}

	puts("");
	dump_sechdr(sec, nsec);

	for (size_t i = 0; i < nsec; ++i) {
		char name[9];
		memcpy(name, sec[i].s_name, sizeof name - 1);
		name[sizeof name - 1] = '\0';

		if (!strcmp(name, ".rsrc")) {
			puts("");
			dump_rsrc(&sec[i], data, sec[i].s_scnptr + sec[i].s_size);
		}
	}

	puts("");

	return 0;
}

int main(int argc, char **argv)
{
	FILE *f;
	void *data;
	size_t size;
	long ssize;

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "pedump");
		return 1;
	}

	if (!(f = fopen(argv[1], "rb"))) {
		perror(argv[1]);
		return 1;
	}

	if (fseek(f, 0, SEEK_END) || (ssize = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)
		|| !(data = malloc(size = ssize)) || fread(data, 1, size, f) != size)
	{
		perror(argv[1]);
		fclose(f);
		return 1;
	}

	fclose(f);
	printf("%s:size=%zX\n", argv[1], size);

	int ret = dump_pe(data, size);
	free(data);
	return ret;
}
