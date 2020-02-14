#include "pe.hpp"

#include <cassert>

#include <stdexcept>
#include <string>

namespace genie {

#define RSRC_DIR 0x80000000

size_t io::peopthdr::size() const {
	return hdr32.coff.o_magic == 0x10b ? sizeof hdr32 : sizeof hdr64;
}

size_t io::penthdr::hdrsz() const {
	return sizeof *this - sizeof(io::peopthdr) + this->OptionalHeader.size();
}

PE::PE(const std::string &name, iofd fd) : blob(name, fd, true), sec_rsrc(nullptr), rsrc(nullptr) {
	// FIXME stub
	size_t size = blob.length();

	if (size < sizeof(io::mz))
		throw std::runtime_error("PE: bad dos header: file too small");

	// probably a COM file or newer
	type = io::PE_Type::mz;
	bits = 16;

	dos = (io::dos*)blob.get();

	if (dos->mz.e_magic != io::dos_magic)
		throw std::runtime_error(std::string("PE: bad dos magic: expected ") + std::to_string(io::dos_magic) + ", got " + std::to_string(dos->mz.e_magic));

	size_t start = dos->mz.e_cparhdr * 16;

	if (start >= size)
		throw std::runtime_error(std::string("PE: bad exe start: got ") + std::to_string(start) + ", max " + std::to_string(size));

	off_t data_start = dos->mz.e_cp * 512;

	if (dos->mz.e_cblp && (data_start -= 512 - dos->mz.e_cblp) < 0)
		throw std::runtime_error(std::string("PE: bad data_start: ") + std::to_string(data_start));

	if (start < sizeof *dos)
		return;

	if (start >= size)
		throw std::runtime_error("PE: bad dos start: file too small");

	// probably MS_DOS file or newer
	type = io::PE_Type::dos;
	dos = (io::dos*)blob.get();

	size_t pe_start = dos->e_lfanew;

	if (pe_start >= size)
		throw std::runtime_error(std::string("PE: bad pe_start: got ") + std::to_string(start) + " max " + std::to_string(size - 1));

	if (size < sizeof *pe + pe_start)
		throw std::runtime_error("PE: bad pe/coff header: file too small");

	type = io::PE_Type::pe;
	bits = 32;

	pe = (io::pehdr*)((char*)blob.get() + pe_start);

	if (pe->f_magic != io::pe_magic)
		throw std::runtime_error(std::string("PE: bad pe magic: expected ") + std::to_string(io::pe_magic) + " got " + std::to_string(pe->f_magic));

	if (!pe->f_opthdr)
		return;

	if (size < sizeof *pe + pe_start + sizeof *peopt)
		throw std::runtime_error("PE: bad pe/coff header: file too small");

	// this is definitely a winnt file
	type = io::PE_Type::peopt;
	peopt = (io::peopthdr*)((char*)blob.get() + pe_start + sizeof *pe);

	switch (peopt->hdr32.coff.o_magic) {
	case 0x10b:
		bits = 32;
		nrvasz = peopt->hdr32.o_nrvasz;
		break;
	case 0x20b:
		bits = 64;
		nrvasz = peopt->hdr64.o_nrvasz;
		break;
	default:
		throw std::runtime_error(std::string("PE: unknown architecture ") + std::to_string(peopt->hdr32.coff.o_magic));
	}

	const io::penthdr *nt = (io::penthdr*)((char*)blob.get() + pe_start);

	ddir = (io::peddir*)(nt + nt->hdrsz());
	size_t sec_start = pe_start + sizeof *pe + pe->f_opthdr;
	sec = (io::sechdr*)((char*)blob.get() + sec_start);
}

bool PE::load_res(io::RsrcType type, res_id id, void **ptr, size_t &count) {
	const void *data = blob.get();
	size_t size = blob.length();
	unsigned language_id = 0; // TODO support multiple languages

	// ensure resource section is loaded
	if (!rsrc) {
		for (unsigned i = 0, n = pe->f_nsect; i < n; ++i) {
			char name[9];
			memcpy(name, sec[i].s_name, sizeof name - 1);
			name[sizeof name - 1] = '\0';

			if (!strcmp(name, ".rsrc")) {
				const io::rsrctbl *rsrc = (io::rsrctbl*)((char*)data + sec[i].s_scnptr);
				if ((char*)&rsrc[1] > (char*)data + size) {
					fprintf(stderr, "%s: bad rsrc: too small\n", __func__);
					return false;
				}

				this->rsrc = rsrc;
				sec_rsrc = &sec[i];
				break;
			}
		}

		if (!this->rsrc) {
			fprintf(stderr, "%s: rsrc not found\n", __func__);
			return false;
		}
	}

	time_t time = rsrc->r_time;
	const io::rsrcditem *item = (io::rsrcditem*)&rsrc[1];

	if (rsrc->r_nname) {
		fprintf(stderr, "%s: skipping %u\n", __func__, rsrc->r_nname);
		item += rsrc->r_nname;
	}

	uint32_t t_id = rsrc->r_nid;

	for (uint32_t i = 0; i < rsrc->r_nid; ++i, ++item)
		// traverse types (level one)
		if (item->r_id == (uint32_t)type) {
			t_id = i;
			break;
		}

	if (t_id == rsrc->r_nid) {
		fprintf(stderr, "%s: res %u not found\n", __func__, id);
		return false;
	}

	if (!(item->r_rva & RSRC_DIR)) {
		fprintf(stderr, "%s: unexpected type leaf: %X\n", __func__, item->r_rva);
		return false;
	}

	uint32_t rva = item->r_rva & ~RSRC_DIR;

	// level two
	const io::rsrctbl *dirtbl = (io::rsrctbl*)((char*)rsrc + rva);

	// skip named items
	const io::rsrcditem *diritem = (io::rsrcditem*)&dirtbl[1];

	if (dirtbl->r_nname) {
		fprintf(stderr, "%s: skipping %u named dirs\n", __func__, dirtbl->r_nname);
		diritem += dirtbl->r_nname;
	}

	// find directory
	unsigned dirid = dirtbl->r_nid;

	for (unsigned i = 0; i < dirtbl->r_nid; ++i, ++diritem) {
		if (!(diritem->r_rva & RSRC_DIR)) {
			fprintf(stderr, "%s: unexpected identifier leaf: %X\n", __func__, diritem->r_rva);
			return false;
		}

		if (diritem->r_id == id || (type == io::RsrcType::string && diritem->r_id - 1 == id / 16)) {
			dirid = i;
			break;
		}
	}

	if (dirid == dirtbl->r_nid) {
		fprintf(stderr, "%s: res %u not found\n", __func__, id);
		return false;
	}

	rva = diritem->r_rva & ~RSRC_DIR;

	const io::rsrctbl *langtbl = (io::rsrctbl*)((char*)rsrc + rva);
	const io::rsrcditem *langitem = (io::rsrcditem*)&langtbl[1];
	unsigned langid = langtbl->r_nid;

	for (unsigned i = 0; i < langtbl->r_nid; ++i, ++langitem) {
		if (langitem->r_rva & RSRC_DIR) {
			fprintf(stderr, "%s: unexpected language id dir\n", __func__);
			return 1;
		}

		if (!language_id || langitem->r_id == language_id) {
			langid = i;
			break;
		}
	}

	if (langid == langtbl->r_nid) {
		fprintf(stderr, "%s: res %u not found\n", __func__, id);
		return false;
	}

	const io::rsrcentry *entry = (io::rsrcentry*)((char*)rsrc + langitem->r_rva);
	off_t e_addr = sec_rsrc->s_scnptr + entry->e_addr - sec_rsrc->s_vaddr;

	if (type != io::RsrcType::string) {
		*ptr = (char*)data + e_addr;
		count = entry->e_size;
		return true;
	}

	// special case for string table, don't ask me why it is designed like this...
	uint16_t strid = (diritem->r_id - 1) * 16;
	const uint16_t *strptr = (uint16_t*)((char*)data + e_addr), *end = (uint16_t*)((char*)data + size - 2);

	for (unsigned i = 0; i < 16; ++i, ++strid) {
		const io::pestr *str = (io::pestr*)strptr;

		if (strptr > end || strptr + str->length > end) {
			fprintf(stderr, "%s: bad string table: unexpected EOF\n", __func__);
			return false;
		}

		++strptr;

		if (strid == id) {
			*ptr = (void*)strptr;
			count = str->length;
			return true;
		} else {
			strptr += str->length;
		}
	}

	fprintf(stderr, "%s: string %u not found\n", __func__, id);
	return false;
}

void PE::load_string(std::string &s, res_id id) {
	void *ptr;
	size_t count;

	if (!load_res(io::RsrcType::string, id, &ptr, count))
		throw std::runtime_error(std::string("Could not load text id ") + std::to_string(id));

	s.clear();

	if (!count)
		return;

	const uint16_t *data = (uint16_t*)ptr;
	s.resize(count);

	for (size_t i = 0; i < count; ++i) {
		unsigned ch = data[i];

		if (ch > UINT8_MAX) {
			// try to convert character
			switch (ch) {
			case 0x2122: ch = 0x99; break;
			default: fprintf(stderr, "%s: truncate ch %X\n", __func__, ch); break;
			}
		}

		s[i] = ch;
	}
}

}
