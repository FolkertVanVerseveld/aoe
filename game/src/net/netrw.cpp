#include "../server.hpp"

#include <except.hpp>
#include <string>

#include <climits>

namespace aoe {

void NetPkg::chktype(NetPkgType type) {
	ntoh();

	if ((NetPkgType)hdr.type == type)
		return;

	switch (type) {
	case NetPkgType::set_protocol:
		throw std::runtime_error("not a protocol network packet");
	case NetPkgType::entity_mod:
		throw std::runtime_error("not an entity control packet");
	case NetPkgType::cam_set:
		throw std::runtime_error("not a camera set packet");
	case NetPkgType::set_username:
		throw std::runtime_error("not a username packet");
	case NetPkgType::chat_text:
		throw std::runtime_error("not a chat text packet");
	case NetPkgType::set_scn_vars:
		throw std::runtime_error("not a scenario settings variables packet");
	case NetPkgType::terrainmod:
		throw std::runtime_error("not a terrain control packet");
	case NetPkgType::peermod:
		throw std::runtime_error("not a peer control packet");
	case NetPkgType::client_info:
		throw std::runtime_error("not a client info packet");
	case NetPkgType::start_game:
		throw std::runtime_error("not a start game packet");
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
			case 'i': case 'I': case 'F': { // int32/uint32
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

				// FIXME this looks like a bug... shouldn't this be: sz = mult
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

void NetPkg::clear() {
	data.clear();
}

netArgsStatus NetPkg::writef(const char *fmt, ...) {
	unsigned mult = 0, size = 0;
	va_list args;
	netArgsStatus r = netArgsStatus::ok;
	size_t sz; // size for string/blob (like a pascal string)
	const unsigned char *ptr;

	va_start(args, fmt);

	for (unsigned ch; (ch = *fmt) != '\0'; ++fmt) {
		if (ch >= '0' && ch <= '9') {
			if (mult > UINT_MAX / 10) {
				r = netArgsStatus::mult_overflow;
				goto end;
			}

			mult = 10 * mult + (ch - '0');
			continue;
		}

		bool has_mult = true;

		if (!mult) {
			has_mult = false;
			mult = 1;
		}

		switch (ch) {
		case 'b': case 'B': // int8/uint8
			for (unsigned i = 0; i < mult; ++i) {
				uint8_t v = (uint8_t)va_arg(args, unsigned);
				data.emplace_back(v);
			}

			size += mult;
			break;
		case 'h': case 'H': // int16/uint16
			for (unsigned i = 0; i < mult; ++i) {
				uint16_t v = (uint16_t)va_arg(args, unsigned);
				data.emplace_back(v >> 8);
				data.emplace_back(v & 0xff);
			}

			size += 2 * mult;
			break;
		case 'i': case 'I': // int32/uint32
			for (unsigned i = 0; i < mult; ++i) {
				uint32_t v = va_arg(args, unsigned);
				data.emplace_back(v >> 24);
				data.emplace_back(v >> 16);
				data.emplace_back(v >> 8);
				data.emplace_back(v & 0xff);
			}

			size += 4 * mult;
			break;
		case 'l': case 'L': // int64/uint64
			for (unsigned i = 0; i < mult; ++i) {
				uint64_t v = va_arg(args, uint64_t);
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
		case 'F': // truncated float (cast to int32)
			for (unsigned i = 0; i < mult; ++i) {
				int32_t v = (int32_t)va_arg(args, double);
				data.emplace_back(v >> 24);
				data.emplace_back(v >> 16);
				data.emplace_back(v >> 8);
				data.emplace_back(v & 0xff);
			}

			size += 4 * mult;
			break;
		case 's': // C string (reads two arguments!). can be used for writing blobs as well
			if (!has_mult) {
				r = netArgsStatus::str_unterminated;
				goto end;
			}

			sz = va_arg(args, size_t);
			if (sz > UINT16_MAX) {
				r = netArgsStatus::str_too_big;
				goto end;
			}

			data.emplace_back(sz >> 8);
			data.emplace_back(sz & 0xff);

			ptr = va_arg(args, const unsigned char*);

			for (unsigned i = 0; i < sz; ++i)
				data.emplace_back(ptr[i]);

			size += 2 + sz;
			break;
		default:
			r = netArgsStatus::bad_fmt_char;
			goto end;
		}

		mult = 0;
	}

end:
	va_end(args);

	(void)size; // TODO if unused, just remove this
	return r;
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

float NetPkg::F32(unsigned pos) const {
	return (float)std::get<uint64_t>(args.at(pos));
}

uint64_t NetPkg::u64(unsigned pos) const {
	return std::get<uint64_t>(args.at(pos));
}

std::string NetPkg::str(unsigned pos) const {
	return std::get<std::string>(args.at(pos));
}

}
