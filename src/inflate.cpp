/* Copyright 2019 Folkert van Verseveld, zlibcomplete by Rudi Cilibrasi. License: MIT */

#include <string.h>
#include <stdint.h>

#define MINIZ_HEADER_FILE_ONLY 1
#include "miniz.c"

#ifndef ZLIB_COMPLETE_CHUNK
#define ZLIB_COMPLETE_CHUNK 16384
#endif

#include <exception>
#include <iostream>
#include <string>
#include <istream>
#include <sstream>

/*
 * Verbatim copy of multiple files of zlibcomplete source code.
 * Some minor refactorings here and there...
 * Copied by Folkert van Verseveld
 * Originally written by Rudi Cilibrasi
 * 
 * This file is licensed MIT.
 * The other files are still AGPLv3!
 */
namespace zlibcomplete {
	enum flush_parameter { no_flush = 0, auto_flush = 1 };

	class ZLibBaseCompressor {
		char in_[ZLIB_COMPLETE_CHUNK];
		char out_[ZLIB_COMPLETE_CHUNK];
		bool autoFlush_;
		bool finished_;
		z_stream strm_;
		protected:
		ZLibBaseCompressor(int level, flush_parameter autoFlush, int windowBits);
		~ZLibBaseCompressor(void);
		std::string baseCompress(const std::string& input);
		std::string baseFinish(void);
	};

	class ZLibBaseDecompressor {
		char in_[ZLIB_COMPLETE_CHUNK];
		char out_[ZLIB_COMPLETE_CHUNK];
		z_stream strm_;
		protected:
		ZLibBaseDecompressor(int windowBits);
		~ZLibBaseDecompressor(void);
		std::string baseDecompress(const std::string& input);
	};

	/**
	 * @brief Raw ZLib compression class using std::string for input and output.
	 *
	 * Implements raw zlib compression using std::string for input and output.
	 * To use, simply call compress() zero or more times and then call finish().
	 * This class does not write a header, so it is necessary to match up the
	 * window_bits parameter for the inflate and deflate methods.
	 * This class implements
	 * <a href="https://www.ietf.org/rfc/rfc1951.txt">RFC 1951.</a>
	 */
	class RawDeflater : public ZLibBaseCompressor {
	public:
		/**
		 * Accepts the standard three parameters for compression constructors.
		 * @param level Compression level 1-9, defaulting to 9, the best compression.
		 * @param flush_parameter Flush mode, either auto_flush or no_flush,
		 *   defaulting to auto_flush mode.
		 * @param window_bits Size of history window between 8 and 15, defaulting
		 *   to 15.  It is important to pass the same number to the RawInflater
		 *   constructor.
		 */
		RawDeflater(int level = 9, flush_parameter autoFlush = auto_flush, int windowBits = 15);
		~RawDeflater(void);
		/**
		 * @brief Compresses data.  Uses std::string for input and output.
		 *
		 * Accepts a std::string of any size containing data to compress.  Returns
		 * as much raw zlib compressed data as possible.  In auto_flush mode, this
		 * will ensure that the returned data is immediately decompressible to the
		 * original data.  In no_flush mode, the data may not all be available
		 * until the rest of the compressed data is written using more compress()
		 * or a finish() call.  After calling compress() zero or more times, it
		 * is necessary to call finish() exactly once.
		 * @param input Any amount of data to compress.
		 * @retval std::string containing compressed data.
		 */
		std::string deflate(const std::string& input);
		/**
		 * @brief Writes a termination block to the raw zlib stream indicating the end.
		 *
		 * Flushes any remaining data that has not yet been written and then
		 * writes a terminal block to the compressed data stream.  Returns this
		 * compressed data as a std::string.  This function should be called exactly
		 * once for each compressed data stream.
		 * @retval std::string containing the last piece of compressed data.
		 */
		std::string finish(void);
	};

	/**
	 * @brief Raw ZLib decompression class using std::string for input and output.
	 *
	 * Implements zlib decompression using std::string for input and output.
	 * To use, simply call decompress() zero or more times.  Because the raw
	 * compressed stream has no header, it is necessary to ensure that the same
	 * window_bits parameter is used for the RawDeflater and RawInflater.
	 * This class implements
	 * <a href="https://www.ietf.org/rfc/rfc1951.txt">RFC 1951.</a>
	 */
	class RawInflater : public ZLibBaseDecompressor {
	public:
		/**
		 * @brief Constructs a Raw ZLib decompressor.
		 *
		 * Accepts a number of window_bits that must match that used with the
		 * RawDeflater.
		 *
		 * @param window_bits An integer between 8 and 15 to control the window_size.
		 * Defaults to 15.  This must match the window_bits used in the RawDeflater.
		 */
		RawInflater(int windowBits = 15);
		~RawInflater(void);
		/**
		 * Accepts a std::string of any size containing compressed data.  Returns
		 * as much zlib uncompressed data as possible.  Call this function over
		 * and over with all the compressed data in a stream in order to decompress
		 * the entire stream.
		 * @param input Any amount of data to decompress.
		 * @retval std::string containing the decompressed data.
		 */
		std::string inflate(const std::string& input);
	};

	RawDeflater::RawDeflater(int level, flush_parameter autoFlush, int windowBits)
		: ZLibBaseCompressor(level, autoFlush, -windowBits) {}

	std::string RawDeflater::deflate(const std::string& input) {
		return baseCompress(input);
	}

	std::string RawDeflater::finish(void) {
		return baseFinish();
	}

	RawDeflater::~RawDeflater(void) {}

	RawInflater::RawInflater(int window_bits) : ZLibBaseDecompressor(-window_bits) {}

	RawInflater::~RawInflater(void) {}

	std::string RawInflater::inflate(const std::string& input) {
		return baseDecompress(input);
	}

	ZLibBaseCompressor::ZLibBaseCompressor(int level, flush_parameter autoFlush, int windowBits) {
		int retval;
		finished_ = false;
		autoFlush_ = autoFlush;
		strm_.zalloc = Z_NULL;
		strm_.zfree = Z_NULL;
		strm_.opaque = Z_NULL;

		if (level < 1 || level > 9)
			level = 9;

		const int memLevel = 9;
		retval = deflateInit2(&strm_, level, Z_DEFLATED, windowBits, memLevel, Z_DEFAULT_STRATEGY);
		if (retval != Z_OK)
			throw std::bad_alloc();
	}

	std::string ZLibBaseCompressor::baseCompress(const std::string& input) {
		std::string result;
		int retval;

		if (finished_) {
			std::cerr << "Cannot compress data after calling finish.\n";
			throw std::exception();
		}

		for (uint32_t i = 0; i < input.length(); i += ZLIB_COMPLETE_CHUNK) {
			int howManyLeft = input.length() - i;
			bool isLastRound = (howManyLeft <= ZLIB_COMPLETE_CHUNK);
			int howManyWanted = (howManyLeft > ZLIB_COMPLETE_CHUNK) ? ZLIB_COMPLETE_CHUNK : howManyLeft;

			memcpy(in_, input.data()+i, howManyWanted);
			strm_.avail_in = howManyWanted;
			strm_.next_in = (Bytef *) in_;

			do {
				strm_.avail_out = ZLIB_COMPLETE_CHUNK;
				strm_.next_out = (Bytef *) out_;

				retval = deflate(&strm_, (autoFlush_ && isLastRound) ? Z_SYNC_FLUSH : Z_NO_FLUSH);
				if (retval == Z_STREAM_ERROR)
					throw std::bad_alloc();

				result += std::string(out_, ZLIB_COMPLETE_CHUNK - strm_.avail_out);
			} while (strm_.avail_out == 0);
		}
		return result;
	}

	std::string ZLibBaseCompressor::baseFinish(void) {
		std::string result;
		int retval, have;

		if (finished_) {
			std::cerr << "Cannot call finish more than once.\n";
			throw std::exception();
		}

		finished_ = true;
		strm_.avail_in = 0;
		strm_.next_in = (Bytef*)in_;

		do {
			strm_.avail_out = ZLIB_COMPLETE_CHUNK;
			strm_.next_out = (Bytef*)out_;
			retval = deflate(&strm_, Z_FINISH);
			if (retval == Z_STREAM_ERROR)
				throw std::bad_alloc();

			have = ZLIB_COMPLETE_CHUNK - strm_.avail_out;
			result += std::string(out_, have);
		} while (strm_.avail_out == 0);

		return result;
	}

	ZLibBaseCompressor::~ZLibBaseCompressor(void) {
		deflateEnd(&strm_);
	}

	ZLibBaseDecompressor::ZLibBaseDecompressor(int windowBits) {
		int retval;

		strm_.zalloc = Z_NULL;
		strm_.zfree = Z_NULL;
		strm_.opaque = Z_NULL;
		strm_.avail_in = 0;
		strm_.next_in = Z_NULL;

		if ((retval = inflateInit2(&strm_, windowBits)) != Z_OK)
			throw std::bad_alloc();
	}

	ZLibBaseDecompressor::~ZLibBaseDecompressor(void) {
		inflateEnd(&strm_);
	}

	std::string ZLibBaseDecompressor::baseDecompress(const std::string& input) {
		int retval;
		std::string result;

		for (uint32_t i = 0; i < input.length(); i += ZLIB_COMPLETE_CHUNK) {
			int howManyLeft = input.length() - i;
			int howManyWanted = (howManyLeft > ZLIB_COMPLETE_CHUNK) ? ZLIB_COMPLETE_CHUNK : howManyLeft;

			memcpy(in_, input.data() + i, howManyWanted);
			strm_.avail_in = howManyWanted;
			strm_.next_in = (Bytef *) in_;

			if (strm_.avail_in == 0)
				break;

			do {
				strm_.avail_out = ZLIB_COMPLETE_CHUNK;
				strm_.next_out = (Bytef*)out_;

				if ((retval = inflate(&strm_, Z_NO_FLUSH)) == Z_STREAM_ERROR)
					throw std::bad_alloc();

				result += std::string(out_, ZLIB_COMPLETE_CHUNK - strm_.avail_out);
			} while (strm_.avail_out == 0);
		}

		return result;
	}

} // namespace zlib_complete

// XXX not the most efficient, but good enough for now
std::string raw_inflate(const void *src, size_t size)
{
	const int CHUNK = 16384;
	char inbuf[CHUNK];
	int readBytes, window_bits = 15;

	// setup deflate
	zlibcomplete::RawInflater inflater(window_bits);
	std::istringstream in(std::string((const char*)src, size));
	std::string out;

	// decompress
	for (;;) {
		in.read(inbuf, CHUNK);
		readBytes = in.gcount();
		if (readBytes == 0)
			break;

		std::string input(inbuf, readBytes);
		out += inflater.inflate(input);
	}

	return out;
}
