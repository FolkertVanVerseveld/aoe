/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

/**
 * Legacy windows Portable Executable crap
 *
 * NOTE this code does not work on big-endian machines!
 */

#include <cstdint>

#include "fs.hpp"
#include "drs.hpp"

namespace genie {

/* You should never need anything in this namespace */
namespace io {

// begin win32 pe crap
// i've renamed a lot of variables for my own sanity
// source: http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/pefile.html

constexpr uint16_t dos_magic = 0x5a4d;
constexpr uint32_t pe_magic = 0x00004550;

struct mz final {
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
};

struct dos final {
	struct mz mz;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint16_t e_lfanew;
};

// IMAGE FILE HEADER
struct pehdr final {
	uint32_t f_magic;
	uint16_t f_mach;
	uint16_t f_nsect;
	uint32_t f_timdat;
	uint32_t f_symptr;
	uint32_t f_nsyms;
	uint16_t f_opthdr;
	uint16_t f_flags;
};

struct coffopthdr final {
	uint16_t o_magic;
	uint8_t o_major;
	uint8_t o_minor;
	uint32_t o_tsize;
	uint32_t o_dsize;
	uint32_t o_bsize;
	uint32_t o_entry;
	uint32_t o_text;
	uint32_t o_data;
	uint32_t o_image;
};

// IMAGE OPTIONAL HEADER
struct peopthdr32 final {
	coffopthdr coff;
	uint32_t o_alnsec;
	uint32_t o_alnfile;
	uint16_t o_osmajor;
	uint16_t o_osminor;
	uint16_t o_imajor;
	uint16_t o_iminor;
	uint16_t o_smajor;
	uint16_t o_sminor;
	uint32_t o_version;
	uint32_t o_isize;
	uint32_t o_hsize;
	uint32_t o_chksum;
	uint16_t o_sub;
	uint16_t o_dllflags;
	uint32_t o_sres;
	uint32_t o_scomm;
	uint32_t o_hres;
	uint32_t o_hcomm;
	uint32_t o_ldflags;
	uint32_t o_nrvasz;
};

// IMAGE OPTIONAL HEADER PE32+
struct peopthdr64 final {
	// we could have used coffopthdr64 here,
	// but this does not make sense for a 64-bit PE
	uint16_t o_magic;
	uint8_t o_major;
	uint8_t o_minor;
	uint32_t o_tsize;
	uint32_t o_dsize;
	uint32_t o_bsize;
	uint32_t o_entry;
	uint32_t o_text;
	uint64_t o_image;
	uint32_t o_alnsec;
	uint32_t o_alnfile;
	uint16_t o_osmajor;
	uint16_t o_osminor;
	uint16_t o_imajor;
	uint16_t o_iminor;
	uint16_t o_smajor;
	uint16_t o_sminor;
	uint32_t o_version;
	uint32_t o_isize;
	uint32_t o_hsize;
	uint32_t o_chksum;
	int16_t o_sub;
	uint16_t o_dllflags;
	uint64_t o_sres;
	uint64_t o_scomm;
	uint64_t o_hres;
	uint64_t o_hcomm;
	uint32_t o_ldflags;
	uint32_t o_nrvasz;
};

// IMAGE OPTIONAL HEADER
// it's really a misnomer... the header is *not* optional
union peopthdr final {
	peopthdr32 hdr32;
	peopthdr64 hdr64;

	size_t size() const;
};

struct peditem final {
	uint32_t d_addr; // VirtualAddress
	uint32_t d_size; // Size
};

struct peddef final {
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

union peddir final {
	peditem items[16];
	peddef data;
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

struct sechdr final {
	char s_name[8];
	uint32_t s_paddr;
	uint32_t s_vaddr;
	uint32_t s_size;
	uint32_t s_scnptr;
	uint32_t s_relptr;
	uint32_t s_lnnoptr;
	uint16_t s_nreloc;
	uint16_t s_nlnno;
	uint32_t s_flags;
};

struct rsrctbl final {
	uint32_t r_flags;
	uint32_t r_time;
	uint16_t r_major;
	uint16_t r_minor;
	uint16_t r_nname;
	uint16_t r_nid;
};

struct rsrcditem final {
	uint32_t r_id;
	uint32_t r_rva;
};

struct rsrcentry final {
	uint32_t e_addr;
	uint32_t e_size;
	uint32_t e_cp;
	uint32_t e_res;
};

// IMAGE NT HEADERS
struct penthdr final {
	uint32_t Signature;
	pehdr FileHeader;
	peopthdr OptionalHeader;

	size_t hdrsz() const;
};

enum class PE_Type {
	unknown = 0,
	mz = 1,
	dos = 2,
	pe = 3,
	peopt = 4
};

enum class RsrcType {
	unknown = 0,
	cursor = 1,
	bitmap = 2,
	icon = 3,
	menu = 4,
	dialog = 5,
	string = 6,
	fontdir = 7,
	font = 8,
	accelerator = 9,
	rcdata = 10,
	messagetable = 11,
	group_cursor = 12,
	group_icon = 14,
	version = 16,
	dlginclude = 17,
	plugplay = 19,
	vxd = 20,
	anicursor = 21,
	aniicon = 22,
};

#pragma warning(push)
#pragma warning(disable: 4200)

struct pestr final {
	uint16_t length;
	char data[];
};

#pragma warning(pop)

}

/**
 * Wrapper to load Portable Executable resources.
 * This wrapper is always preferred over any OS API for PE resources because it
 * does not care if the PE arch and OS arch match (i.e. we can load resources
 * on a 64 bit arch even though the DLL is 32 bit and vice versa).
 */
class PE {
	Blob blob;
	io::PE_Type type;
	unsigned bits;
	const io::dos *dos;
	const io::pehdr *pe;
	const io::peopthdr *peopt;
	unsigned nrvasz, nrvan;
	const io::peddir *ddir;
	const io::sechdr *sec, *sec_rsrc;
	const io::rsrctbl *rsrc;
public:
	PE(const std::string &name, iofd fd);
	/** Fetch the specified resource item associated with the specified type and id. */
	bool load_res(io::RsrcType type, res_id id, void **data, size_t &count);
	void load_string(std::string &s, res_id id);
};

}
