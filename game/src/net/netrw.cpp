#include "../server.hpp"

#include <except.hpp>
#include <string>

namespace aoe {

void NetPkg::chktype(NetPkgType type) {
	ntoh();

	if ((NetPkgType)hdr.type == type)
		return;

	switch (type) {
	case NetPkgType::set_protocol:
		throw std::runtime_error("not a protocol network packet");
	case NetPkgType::entity_mod:
		throw std::runtime_error("not an entityu control packet");
	case NetPkgType::cam_set:
		throw std::runtime_error("not a camera set packet");
	default:
		throw std::runtime_error("invalid network packet type");
	}
}

unsigned NetPkg::read(NetPkgType type, const std::string &fmt) {
	chktype(type);
	args.clear();
	return read(fmt, args, 0);
}

unsigned NetPkg::read(const std::string &fmt, netargs &dst, unsigned offset) {
	unsigned mult = 0, size = 0;

	for (unsigned char ch : fmt) {
		if (ch >= '0' && ch <= '9') {
			if (mult > UINT_MAX / 10)
				throw std::runtime_error("number overflow");

			mult = 10 * mult + (ch - '0');
			continue;
		}

		bool has_mult = true;

		if (!mult) {
			has_mult = false;
			mult = 1;
		}

		switch (ch) {
			case 'b': case 'B': { // int8/uint8
				for (unsigned i = 0; i < mult; ++i, ++offset)
					dst.emplace_back((uint64_t)data.at(offset));

				size += mult;
				break;
			}
			case 'h': case 'H': { // int16/uint16
				for (unsigned i = 0; i < mult; ++i, offset += 2) {
					uint16_t v = (uint16_t)data.at(offset) << 8 | (uint16_t)data.at(offset + 1);
					dst.emplace_back((uint64_t)v);
				}

				size += 2 * mult;
				break;
			}
			case 'i': case 'I': { // int32/uint32
				for (unsigned i = 0; i < mult; ++i, offset += 4) {
					uint32_t v = (uint32_t)data.at(offset) << 24 | (uint32_t)data.at(offset + 1) << 16
						| (uint32_t)data.at(offset + 2) << 8 | (uint32_t)data.at(offset + 3);
					dst.emplace_back((uint64_t)v);
				}

				size += 4 * mult;
				break;
			}
			case 'l': case 'L': { // int64/uint64
				for (unsigned i = 0; i < mult; ++i, offset += 8) {
					uint64_t v = (uint64_t)data.at(offset) << 56ull | (uint64_t)data.at(offset + 1) << 48ull
						| (uint64_t)data.at(offset + 2) << 40ull | (uint64_t)data.at(offset + 3) << 32ull
						| (uint64_t)data.at(offset + 4) << 24ull | (uint64_t)data.at(offset + 5) << 16ull
						| (uint64_t)data.at(offset + 6) << 8ull | (uint64_t)data.at(offset + 7);
					dst.emplace_back(v);
				}

				size += 8 * mult;
				break;
			}
			case 's': { // std::string (size limited to uint16)
				if (!has_mult)
					throw std::runtime_error("string has no length");

				unsigned sz = std::min<unsigned>(data.at(offset) << 8 | data.at(offset + 1), mult);

				std::string s;

				for (unsigned i = 0; i < sz; ++i)
					s += data.at(offset + 2 + i);

				offset += 2 + sz;
				size += 2 + sz;

				dst.emplace_back(s);
				break;
			}
			default:
				throw std::runtime_error(std::string("bad character 0d") + std::to_string(ch) + " in format");
		}

		mult = 0;
	}

	return size;
}

unsigned NetPkg::write(const std::string &fmt, const netargs &args, bool append) {
	unsigned mult = 0, size = 0, argpos = 0;

	if (!append)
		data.clear();

	for (unsigned char ch : fmt) {
		if (ch >= '0' && ch <= '9') {
			if (mult > UINT_MAX / 10)
				throw std::runtime_error("number overflow");

			mult = 10 * mult + (ch - '0');
			continue;
		}

		bool has_mult = true;

		if (!mult) {
			has_mult = false;
			mult = 1;
		}

		switch (ch) {
			case 'b': case 'B': { // int8/uint8
				for (unsigned i = 0; i < mult; ++i, ++argpos)
					data.emplace_back((uint8_t)std::get<uint64_t>(args.at(argpos)));

				size += mult;
				break;
			}
			case 'h': case 'H': { // int16/uint16
				for (unsigned i = 0; i < mult; ++i, ++argpos) {
					uint16_t v = (uint16_t)std::get<uint64_t>(args.at(argpos));
					data.emplace_back(v >> 8);
					data.emplace_back(v & 0xff);
				}

				size += 2 * mult;
				break;
			}
			case 'i': case 'I': { // int32/uint32
				for (unsigned i = 0; i < mult; ++i, ++argpos) {
					uint32_t v = (uint32_t)std::get<uint64_t>(args.at(argpos));
					data.emplace_back(v >> 24);
					data.emplace_back(v >> 16);
					data.emplace_back(v >> 8);
					data.emplace_back(v & 0xff);
				}

				size += 4 * mult;
				break;
			}
			case 'l': case 'L': { // int64/uint64
				for (unsigned i = 0; i < mult; ++i, ++argpos) {
					uint64_t v = std::get<uint64_t>(args.at(argpos));
					data.emplace_back(v >> 56ull);
					data.emplace_back(v >> 48ull);
					data.emplace_back(v >> 40ull);
					data.emplace_back(v >> 32ull);
					data.emplace_back(v >> 24ull);
					data.emplace_back(v >> 16ull);
					data.emplace_back(v >> 8ull);
					data.emplace_back(v & 0xffull);
				}

				size += 8 * mult;
				break;
			}
			case 's': { // std::string (size limited to uint16)
				if (!has_mult)
					throw std::runtime_error("string has no length");

				std::string s(std::get<std::string>(args.at(argpos++)));
				if (s.size() > UINT16_MAX)
					throw std::runtime_error("string too large");

				uint16_t sz = std::min<unsigned>(mult, s.size());

				data.emplace_back(sz >> 8);
				data.emplace_back(sz & 0xff);

				for (unsigned i = 0; i < sz; ++i)
					data.emplace_back(s[i]);

				size += 2 + sz;
				break;
			}
			default:
				throw std::runtime_error(std::string("bad character 0d") + std::to_string(ch) + " in format");
		}

		mult = 0;
	}

	return size;
}

int8_t NetPkg::i8(unsigned pos) const {
	return (int8_t)std::get<uint64_t>(args.at(pos));
}

uint8_t NetPkg::u8(unsigned pos) const {
	return (uint8_t)std::get<uint64_t>(args.at(pos));
}

uint16_t NetPkg::u16(unsigned pos) const {
	return (uint16_t)std::get<uint64_t>(args.at(pos));
}

int32_t NetPkg::i32(unsigned pos) const {
	return (int32_t)std::get<uint64_t>(args.at(pos));
}

uint32_t NetPkg::u32(unsigned pos) const {
	return (uint32_t)std::get<uint64_t>(args.at(pos));
}

uint64_t NetPkg::u64(unsigned pos) const {
	return std::get<uint64_t>(args.at(pos));
}

std::string NetPkg::str(unsigned pos) const {
	return std::get<std::string>(args.at(pos));
}

}
