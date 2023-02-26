#include "../legacy.hpp"

#include "../engine/endian.h"

#include <cassert>

namespace aoe {
namespace io {

// begin win32 pe crap
// i've renamed a lot of variables for my own sanity
// source: http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/pefile.html

static constexpr uint16_t dos_magic = 0x5a4d;
static constexpr uint32_t pe_magic = 0x00004550;

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

	size_t size() const noexcept {
		return hdr32.coff.o_magic == 0x10b ? sizeof hdr32 : sizeof hdr64;
	}
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

struct rsrctbl final {
	uint32_t r_flags;
	uint32_t r_time;
	uint16_t r_major;
	uint16_t r_minor;
	uint16_t r_nname;
	uint16_t r_nid;
};

#define RSRC_DIR 0x80000000

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

	size_t size() const noexcept {
		return sizeof *this - sizeof(OptionalHeader) + OptionalHeader.size();
	}
};

#pragma warning(push)
#pragma warning(disable: 4200)

struct pestr final {
	uint16_t length;
	char data[];
};

#pragma warning(pop)

PE::PE(const std::string &path) : in(path, std::ios_base::binary), m_type(PE_Type::unknown), bits(0), nrvasz(0), sections(), path(path) {
	in.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	in.seekg(0, std::ios_base::end);
	long long end = in.tellg();
	in.seekg(0);
	long long beg = in.tellg();

	assert(end >= beg);
	size_t size = (unsigned long long)(end - beg);

	mz mz;
	in.read((char*)&mz, sizeof mz);

	// only fix the endian byte order of variables we care about
	mz.e_magic = le16toh(mz.e_magic);
	if (mz.e_magic != dos_magic)
		throw std::runtime_error(std::string("PE: bad dos magic: expected ") + std::to_string(dos_magic) + ", got " + std::to_string(mz.e_magic));

	size_t start = mz.e_cparhdr * 16;

	m_type = PE_Type::mz;
	bits = 16;

	if (start >= size)
		throw std::runtime_error(std::string("PE: bad exe start: got ") + std::to_string(start) + ", max " + std::to_string(size));

	off_t data_start = mz.e_cp * 512;

	if (mz.e_cblp && (data_start -= 512 - mz.e_cblp) < 0)
		throw std::runtime_error(std::string("PE: bad data_start: ") + std::to_string(data_start));

	if (start < sizeof(dos))
		return;

	in.seekg(0);
	dos dos;
	in.read((char*)&dos, sizeof dos);

	dos.e_lfanew = le16toh(dos.e_lfanew);
	size_t pe_start = dos.e_lfanew;

	if (pe_start >= size)
		throw std::runtime_error(std::string("PE: bad pe_start: got ") + std::to_string(start) + " max " + std::to_string(size - 1));

	pehdr pe;

	if (size < sizeof(pe) + pe_start)
		throw std::runtime_error("PE: bad pe/coff header: file too small");

	m_type = PE_Type::pe;
	bits = 32;

	in.seekg(pe_start);
	in.read((char*)&pe, sizeof pe);

	// only fix the endian byte order of variables we care about
	pe.f_magic = le32toh(pe.f_magic);
	pe.f_opthdr = le16toh(pe.f_opthdr);
	pe.f_nsect = le16toh(pe.f_nsect);

	if (pe.f_magic != pe_magic)
	throw std::runtime_error(std::string("PE: bad pe magic: expected ") + std::to_string(pe_magic) + " got " + std::to_string(pe.f_magic));

	if (!pe.f_opthdr)
		return;

	coffopthdr coff;

	if (size < sizeof(pe) + pe_start + sizeof(coff))
		throw std::runtime_error("PE: bad pe/coff header: file too small");

	// this is definitely a winnt file
	m_type = io::PE_Type::peopt;
	in.seekg(pe_start + sizeof(pe));
	in.read((char*)&coff, sizeof coff);
	in.seekg(pe_start + sizeof(pe));

	peopthdr peopt;

	coff.o_magic = le16toh(coff.o_magic);
	switch (coff.o_magic) {
	case 0x10b:
		in.read((char*)&peopt.hdr32, sizeof(peopt.hdr32));
		bits = 32;
		nrvasz = le32toh(peopt.hdr32.o_nrvasz);
		break;
	case 0x20b:
		in.read((char*)&peopt.hdr64, sizeof(peopt.hdr64));
		bits = 64;
		nrvasz = le32toh(peopt.hdr64.o_nrvasz);
		break;
	default:
		throw std::runtime_error(std::string("PE: unknown architecture ") + std::to_string(coff.o_magic));
	}

	in.seekg(pe_start);

	penthdr nt;
	in.read((char*)&nt, sizeof nt);

	size_t sec_start = pe_start + sizeof(pe) + pe.f_opthdr;
	in.seekg(sec_start);

	sections.resize(pe.f_nsect);
	in.read((char*)sections.data(), sections.size() * sizeof sechdr);
}

bool PE::load_res(RsrcType type, res_id id, size_t &pos, size_t &count) {
	unsigned language_id = 0; // TODO support multiple languages

	// locate .rsrc section
	uint32_t rsrc_pos = 0;
	const sechdr *rsrc_sec = nullptr;

	for (const sechdr &sec : sections) {
		if (!strcmp(sec.s_name, ".rsrc")) {
			rsrc_pos = le32toh(sec.s_scnptr);
			rsrc_sec = &sec;
			break;
		}
	}

	if (!rsrc_pos) {
		fprintf(stderr, "%s: rsrc not found\n", __func__);
		return false;
	}

	// level one
	rsrctbl rsrc;
	read((char*)&rsrc, rsrc_pos, sizeof rsrc);

	time_t time = le32toh(rsrc.r_time);

	std::vector<rsrcditem> l1(le32toh(rsrc.r_nid));
	long long item_pos = rsrc_pos + sizeof rsrc;

	read((char*)l1.data(), item_pos + le32toh(rsrc.r_nname) * sizeof rsrcditem, l1.size() * sizeof rsrcditem);

	// find type directory
	uint32_t t_id = l1.size();

	for (uint32_t i = 0; i < l1.size(); ++i) {
		if (le32toh(l1[i].r_id) == (uint32_t)type) {
			t_id = i;
			break;
		}
	}

	rsrcditem &item = l1.at(t_id);

	if (!(le32toh(item.r_rva) & RSRC_DIR)) {
		fprintf(stderr, "%s: unexpected type leaf: %X\n", __func__, le32toh(item.r_rva));
		return false;
	}

	uint32_t rva = le32toh(item.r_rva) & ~RSRC_DIR;

	// level two
	rsrctbl dirtbl;
	long long dir_pos = rsrc_pos + rva;
	read((char*)&dirtbl, dir_pos, sizeof dirtbl);

	std::vector<rsrcditem> l2(le32toh(dirtbl.r_nid));

	read((char*)l2.data(), dir_pos + sizeof dirtbl + le32toh(dirtbl.r_nname) * sizeof rsrcditem, l2.size() * sizeof rsrcditem);

	// find name directory
	unsigned dirid = le32toh(dirtbl.r_nid);

	for (unsigned i = 0; i < le32toh(dirtbl.r_nid); ++i) {
		rsrcditem &d = l2[i];
		if (!(le32toh(d.r_rva) & RSRC_DIR)) {
			fprintf(stderr, "%s: unexpected identifier leaf: %X\n", __func__, le32toh(d.r_rva));
			return false;
		}

		if (le32toh(d.r_id) == id || (type == RsrcType::string && le32toh(d.r_id) - 1 == id / 16)) {
			dirid = i;
			break;
		}
	}

	item = l2.at(dirid);
	rva = item.r_rva & ~RSRC_DIR;
	rsrcditem diritem = item;

	// level three
	long long lang_pos = rsrc_pos + rva;
	rsrctbl langtbl;

	read((char*)&langtbl, lang_pos, sizeof langtbl);

	std::vector<rsrcditem> l3(le32toh(langtbl.r_nid));

	read((char*)l3.data(), lang_pos + sizeof langtbl + le32toh(langtbl.r_nname) * sizeof rsrcditem, l3.size() * sizeof rsrcditem);

	// find language directory
	uint32_t langid = le32toh(langtbl.r_nid);

	for (unsigned i = 0; i < le32toh(langtbl.r_nid); ++i) {
		rsrcditem &d = l3[i];

		if (le32toh(d.r_rva) & RSRC_DIR) {
			fprintf(stderr, "%s: unexpected language id dir\n", __func__);
			return false;
		}

		// TODO when supporting multiple languages, remove `!language_id ||'
		if (!language_id || d.r_id == language_id) {
			langid = i;
			break;
		}
	}

	item = l3.at(langid);

	rsrcentry entry;
	read((char*)&entry, rsrc_pos + le32toh(item.r_rva), sizeof entry);

	off_t e_addr = le32toh(rsrc_sec->s_scnptr) + le32toh(entry.e_addr) - le32toh(rsrc_sec->s_vaddr);

	if (type != RsrcType::string) {
		pos = e_addr;
		count = le32toh(entry.e_size);
		return true;
	}

	uint16_t strid = (le32toh(diritem.r_id) - 1) * 16;
	long long strpos = e_addr;

	for (unsigned i = 0; i < 16; ++i, ++strid) {
		pestr str;
		read((char*)&str, strpos, 2);

		strpos += 2;

		// times 2 because ntkernel uses UCS2 which is 2 bytes per code unit.
		size_t size = 2 * le16toh(str.length);

		if (strid == id) {
			pos = strpos;
			count = size;
			return true;
		}

		strpos += size;
	}

	// not found
	return false;
}

void PE::read(char *dst, size_t pos, size_t size) {
	in.seekg(pos, std::ios_base::beg);
	in.read(dst, size);
}

std::string PE::load_string(res_id id) {
	size_t pos, count;

	if (!load_res(io::RsrcType::string, id, pos, count))
		throw std::runtime_error(std::string("Could not load text id ") + std::to_string(id));

	if (!count)
		return "";

	std::vector<uint16_t> data(count / 2, 0);
	read((char*)data.data(), pos, count);

	std::string s;

	for (size_t i = 0; i < count / 2; ++i) {
		unsigned ch = data[i];

		if (ch > UINT8_MAX) {
			// try to convert character
			// TODO broken :/
			switch (ch) {
			case 0xa9: ch = 0xae; break; // copyright
			case 0x2122: ch = 0xae; break;// ch = 0x99; break; // trademark
			default: fprintf(stderr, "%s: truncate ch %X\n", __func__, ch); break;
			}
		}

		s += ch;
	}

	return s;
}

}
}
