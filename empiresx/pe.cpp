#include "pe.hpp"

#include <cassert>

#include <stdexcept>
#include <string>

namespace genie {

size_t io::peopthdr::size() const {
	return hdr32.coff.o_magic == 0x10b ? sizeof hdr32 : sizeof hdr64;
}

size_t io::penthdr::hdrsz() const {
	return sizeof *this - sizeof(io::peopthdr) + this->OptionalHeader.size();
}

PE::PE(const std::string &name, iofd fd) : blob(name, fd, true) {
	// FIXME stub
	size_t size = blob.length();

	if (size < sizeof io::mz)
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

}
