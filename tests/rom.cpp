#include "rom.h"

#include "endian.hpp"
#include "rom_debug.hpp"
#include "string_utils.hpp"

#include <array>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <span>
#include <sstream>


namespace builtin_rom
{

const genesis::rom::byte_array empty_array = {};


const genesis::rom::vector_array raw_vectors = {
	0x00fffff6, 0x00000200, 0x00002460, 0x0000246e, 0x0000247c, 0x0000248a, 0x00002498, 0x000024a6,
	0x000024b4, 0x000024c2, 0x000024d0, 0x000024de, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec,
	0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec,
	0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024fa, 0x000024ec, 0x000021b6, 0x000024ec,
	0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec,
	0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec,
	0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec,
	0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec, 0x000024ec};


const std::array<std::uint8_t, 256> raw_header = {
	// sys type
	0x53, 0x45, 0x47, 0x41, 0x20, 0x47, 0x45, 0x4e, 0x45, 0x53, 0x49, 0x53, 0x20, 0x20, 0x20, 0x20,

	// copyright
	0x28, 0x43, 0x29, 0x20, 0x47, 0x45, 0x4e, 0x45, 0x53, 0x49, 0x53, 0x20, 0x32, 0x30, 0x32, 0x31,

	// Game name (domestic)
	0x47, 0x61, 0x6d, 0x65, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x20, 0x28, 0x64, 0x6f, 0x6d, 0x65, 0x73, 0x74, 0x69, 0x63,
	0x29, 0x20, 0x2d, 0x20, 0x47, 0x45, 0x4e, 0x45, 0x53, 0x49, 0x53, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x72, 0x6f,
	0x6d, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

	// Game name (overseas)
	0x47, 0x61, 0x6d, 0x65, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x20, 0x28, 0x6f, 0x76, 0x65, 0x72, 0x73, 0x65, 0x61, 0x73,
	0x29, 0x20, 0x2d, 0x20, 0x47, 0x45, 0x4e, 0x45, 0x53, 0x49, 0x53, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x72, 0x6f,
	0x6d, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

	0x47, 0x4d, 0x20, 0x54, 0x2d, 0x35, 0x30, 0x39, 0x36, 0x36, 0x20, 0x2d, 0x30, 0x30,

	// checksum
	0x64, 0x13,

	0x4a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

	// rom address range
	0x12, 0x00, 0x34, 0x00, 0x00, 0x1f, 0xff, 0xff,

	// ram address range
	0x56, 0x00, 0x78, 0x00, 0x00, 0xff, 0xff, 0xff,

	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x55, 0x45, 0x4a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20};


// this reflects raw_header byte array, so any changes here must be propagated to raw data (and vs)
const genesis::rom::header_data header = {.system_type = "SEGA GENESIS",
										  .copyright = "(C) GENESIS 2021",
										  .game_name_domestic = "Game name (domestic) - GENESIS test rom",
										  .game_name_overseas = "Game name (overseas) - GENESIS test rom",
										  .rom_checksum = 0x6413,
										  .rom_start_addr = 0x12003400,
										  .rom_end_addr = 0x001fffff,
										  .ram_start_addr = 0x56007800,
										  .ram_end_addr = 0x00ffffff};


// random bytes, checksum: 0x64, 0x13
const genesis::rom::byte_array raw_body = {
	0xe0, 0x9b, 0x34, 0xd9, 0xae, 0x62, 0x7c, 0xa5, 0xd9, 0x9a, 0x3c, 0x93, 0x01, 0x84, 0xf1, 0xa4, 0x09, 0x9a, 0x57,
	0x2b, 0xd0, 0xc7, 0xbd, 0x28, 0x22, 0x69, 0xc1, 0xcd, 0x9a, 0xdb, 0x3c, 0x45, 0x6a, 0xd5, 0x0b, 0x82, 0x38, 0x20,
	0xac, 0x70, 0xa3, 0x8b, 0x7f, 0x50, 0xa9, 0xfa, 0xf1, 0xda, 0x55, 0x5e, 0x5a, 0x8a, 0xf2, 0xd0, 0xe4, 0x3f, 0x1a,
	0xa9, 0x1a, 0x9c, 0xc9, 0x9f, 0x66, 0xf0, 0x84, 0xd4, 0x21, 0x3c, 0x5e, 0x0c, 0x11, 0x76, 0x3c, 0x2b, 0x92, 0x36,
	0x7e, 0x41, 0x8f, 0x63, 0x2c, 0x07, 0x90, 0x21, 0x70, 0xe2, 0x6c, 0xea, 0xef, 0xff, 0xe7, 0xcb, 0x3e, 0x12, 0x2b,
	0xaf, 0x76, 0xc9, 0x9c, 0xf0, 0xc9, 0xf2, 0xb1, 0xe0, 0x4e, 0xc7, 0x23, 0x29, 0x2a, 0x22, 0x4b, 0x16, 0x66, 0x5a,
	0x59, 0xc9, 0x76, 0x4f, 0xa7, 0x87, 0x83, 0x4e, 0xfb, 0x2b, 0xe3, 0xf4, 0xe1, 0xa5, 0xfe, 0xf8, 0x11, 0x3d, 0x45,
	0x5a, 0x6a, 0x6d, 0xf9, 0x55, 0xf7, 0x86, 0xac, 0x4c, 0xdc, 0x21, 0x56, 0xb2, 0xc8, 0x22, 0xcb, 0x88, 0x26, 0xb5,
	0xd1, 0x4c, 0xc7, 0x2b, 0xf1, 0x0a, 0x43, 0xdb, 0x4f, 0x35, 0xcf, 0x43, 0xd6, 0xff, 0x35, 0x17, 0x5b, 0x38, 0x3a,
	0x52, 0x9b, 0xd4, 0x25, 0xa0, 0x64, 0x15, 0xcd, 0x95, 0x84, 0x5d, 0x2a, 0x35, 0x92, 0x7a, 0xa5, 0x47, 0x6f, 0xcc,
	0xeb, 0x21, 0x5d, 0x96, 0x3b, 0x5e, 0x9b, 0x62, 0xcf, 0xe5, 0xcd, 0x88, 0x35, 0x9a, 0x1c, 0x04, 0x5a, 0x50, 0x9c,
	0x51, 0x4f, 0x1a, 0x9a, 0xa1, 0xce, 0xeb, 0xf3, 0x89, 0x73, 0xdf, 0xef, 0xfc, 0x2d, 0xff, 0xd9, 0x05, 0x01, 0x05,
	0x30, 0xf0, 0x54, 0x4a, 0xea, 0x6a, 0x6f, 0x0a, 0x1e, 0xd9, 0xe1, 0xab, 0xf2, 0x54, 0x7b, 0x74, 0xa9, 0xd4, 0x0c,
	0xcf, 0xda, 0x6c, 0x6b, 0x3a, 0xac, 0xdc, 0x33, 0x97, 0x68, 0x99, 0x8f, 0xcb, 0x5f, 0x77, 0x05, 0x9a, 0x2f, 0x78,
	0x71, 0x7b, 0x1d, 0x92, 0xd9, 0xbe, 0x8f, 0x7b, 0x1d, 0x74, 0xf1, 0x19, 0xc8, 0xc1, 0x45, 0x09, 0x40, 0x1a, 0xdd,
	0x84, 0x81, 0x22, 0x94, 0x7c, 0x14, 0x05, 0x2a, 0x91, 0x28, 0xac, 0xea, 0x40, 0x57, 0xc9, 0x19, 0x94, 0x04, 0x46,
	0xe3, 0x7c, 0xaf, 0xe2, 0xdd, 0x57, 0x26, 0xbf, 0x9d, 0x7e, 0xc8, 0xdd, 0x36, 0x4b, 0x13, 0xc7, 0xf4, 0xc0, 0xcb,
	0xdd, 0x8c, 0xc8, 0xa0, 0x37, 0xe3, 0x68, 0x7b, 0x3b, 0xe1, 0x89, 0x4a, 0xf7, 0xe0, 0xd5, 0xec, 0xd1, 0xed, 0x16,
	0x89, 0xc9, 0x40, 0xb6, 0xb6, 0x6d, 0xd0, 0x5e, 0x22, 0x28, 0x0a, 0xf6, 0xe5, 0x86, 0xdc, 0x86, 0x7e, 0xdb, 0xc2,
	0x61, 0x43, 0x2e, 0xd2, 0x89, 0x8b, 0x73, 0x95, 0x4b, 0x0f, 0x03, 0xd9, 0xe6, 0x09, 0x05, 0x34, 0x75, 0xa7, 0x5f,
	0xa5, 0x3d, 0x84, 0xba, 0x44, 0xed, 0x24, 0x81, 0x28, 0xa2, 0x7e, 0x8a, 0x66, 0xf2, 0x26, 0xde, 0x7b, 0x39, 0x00,
	0x26, 0x94, 0x38, 0xb5, 0x4d, 0x7c, 0x4d, 0xb8, 0xd9, 0xe8, 0x8c, 0x6a, 0xc0, 0xde, 0x93, 0x11, 0x3d, 0x26, 0x67,
	0x3b, 0xc9, 0x72, 0xf7, 0xd0, 0xaa, 0x44, 0x57, 0x12, 0xd9, 0xa2, 0xc8, 0xcc, 0x6c, 0x76, 0x51, 0x3b, 0x6d, 0xc3,
	0x85, 0x35, 0xf8, 0x89, 0x63, 0x4b, 0xb4, 0x95, 0xae, 0x57, 0xd0, 0x9c, 0x7d, 0x72, 0xc2, 0xfc, 0xf0, 0xfe, 0x74,
	0xda, 0xcf, 0x18, 0xa9, 0x16, 0x11, 0x55, 0x18, 0x0a, 0xde, 0xae, 0x7f, 0x56, 0xb0, 0x13, 0xa9, 0x09, 0x3d, 0xb2,
	0x12, 0xb3, 0x63, 0xfb, 0x38, 0xdb, 0x61, 0x81, 0xc4, 0xef, 0xd8, 0xf1, 0xbf, 0xb5, 0x00, 0x74, 0x47, 0x9e, 0x7f,
	0x3f, 0x85, 0xec, 0xe7, 0x58, 0xff, 0x69, 0x76, 0xa0, 0x97, 0x65, 0x0a, 0x4a, 0x99, 0xdf, 0x44, 0x04, 0x59, 0xf6,
	0x23, 0xde, 0xb2, 0xb9, 0x91, 0x10, 0x22, 0x1f, 0xe5, 0xca, 0x4d, 0xbe, 0x0e, 0xa5, 0x0c, 0xe6, 0x69, 0x12, 0xcd,
	0x93, 0x6a, 0xfa, 0x0e, 0x55, 0x31, 0xe2, 0x1a, 0x51, 0xb8, 0xa4, 0xee, 0xa9, 0x1b, 0x5d, 0x91, 0x9f, 0xff, 0xe3,
	0x5c, 0x81, 0x15, 0xa6, 0xae, 0xbe, 0x2a, 0xfc, 0x96, 0xee, 0x57, 0x19, 0xc7, 0x6b, 0x75, 0xd5, 0x14, 0xbc, 0x1d,
	0xad, 0xba, 0x7a, 0x4c, 0x2f, 0x9b, 0x93, 0x2a, 0x9b, 0x02, 0x79, 0xb4, 0x55, 0x5c, 0x54, 0x17, 0x32, 0x33, 0x73,
	0x14, 0x24, 0x12, 0x0e, 0x70, 0x5f, 0xcb, 0xb3, 0xf7, 0x0c, 0x7d, 0x49, 0xb5, 0xd3, 0xc9, 0x1b, 0xa8, 0x32, 0xbb,
	0x8f, 0x2f, 0x74, 0xf4, 0x83, 0x02, 0x26, 0xe9, 0xc8, 0x66, 0x9e, 0x35, 0xdf, 0x16, 0x1b, 0xac, 0x6f, 0x74, 0xe1,
	0xf3, 0x64, 0xf7, 0xb3, 0x63, 0x20, 0x37, 0x60, 0x6b, 0xab, 0xa9, 0x6b, 0x3b, 0x67, 0x11, 0x2f, 0x92, 0x61, 0x55,
	0x47, 0xed, 0x64, 0x5a, 0xc7, 0x4e, 0xaf, 0x96, 0x34, 0xfe, 0x15, 0x37, 0x93, 0x33, 0x3d, 0x8b, 0xea, 0xc2, 0x8e,
	0xf5, 0xc8, 0x2b, 0xb3, 0xbc, 0x7e, 0x2c, 0x67, 0x53, 0x3a, 0x80, 0xdc, 0x0e, 0xa6, 0x0e, 0xdd, 0x20, 0xda, 0x47,
	0x49, 0xd6, 0xaa, 0xd1, 0x90, 0xbb, 0x00, 0x81, 0xd4, 0xd2, 0x4a, 0x34, 0x9b, 0x0f, 0xd0, 0xd1, 0x8a, 0x88, 0xc3,
	0x2b, 0xbb, 0xd4, 0xde, 0x6f, 0x25, 0xc4, 0x9b, 0xe8, 0xb1, 0x05, 0xd2, 0x40, 0x5c, 0x3f, 0xd2, 0xfb, 0x4b, 0xaf,
	0xc1, 0x86, 0x31, 0x72, 0x9d, 0xc4, 0x7e, 0xd5, 0xb3, 0x5a, 0x77, 0x5b, 0xd9, 0xca, 0xa5, 0xdc, 0x73, 0x4d, 0x10,
	0x1b, 0x8c, 0x5d, 0x26, 0xb7, 0x21, 0x05, 0xba, 0xa0, 0xf7, 0xbe, 0xdf, 0xc7, 0x4b, 0xe2, 0xce, 0x93, 0xb3, 0x69,
	0x4d, 0x68, 0x44, 0x4b, 0x41, 0xf8, 0x30, 0xe8, 0x72, 0x6d, 0x2c, 0xe7, 0x95, 0xe6, 0xa9, 0xb7, 0xb0, 0xb5, 0x08,
	0x12, 0xed, 0xca, 0xf0, 0x8b, 0xd8, 0x42, 0x9f, 0x84, 0xc6, 0x27, 0x0d, 0x86, 0xbc, 0x40, 0x60, 0x20, 0x39, 0xa7,
	0x7b, 0x5b, 0x41, 0x99, 0xb1, 0x74, 0x90, 0xa3, 0xfc, 0x84, 0x59, 0xb6, 0xdf, 0x4c, 0x90, 0x6b, 0x53, 0x81, 0xd0,
	0x53, 0x06, 0x0e, 0x66, 0x4d, 0xbb, 0x22, 0x9a, 0x1e, 0xd3, 0xa5, 0x21, 0x42, 0xe6, 0xe3, 0xf3, 0x06, 0x1e, 0x68,
	0x57, 0x13, 0x34, 0xf8, 0x8f, 0x6a, 0x49, 0x4b, 0xac, 0xbf, 0xa1, 0x8f, 0xff, 0x5c, 0xcc, 0x8d, 0x6e, 0xcc, 0x11,
	0xa7, 0x16, 0x03, 0x37, 0xdb, 0xa6, 0x2d, 0x2b, 0xfc, 0x84, 0x99, 0x31, 0xc9, 0xfe, 0xfa, 0x52, 0x24, 0x22, 0xd2,
	0xe8, 0x69, 0x8d, 0x06, 0xe6, 0x84, 0x02, 0xdd, 0x98, 0xae, 0x95, 0xf7, 0x38, 0x4e, 0x83, 0x8b, 0x98, 0xfc, 0x72,
	0x9b, 0x24, 0x71, 0xf7, 0x44, 0xb4, 0x59, 0xe1, 0xf0, 0x44, 0xf0, 0xf5, 0x96, 0x5b, 0xf6, 0x55, 0xe0, 0x67, 0x97,
	0xf3, 0xc1, 0xa1, 0xb0, 0x43, 0xf2, 0x38, 0x92, 0x31, 0x20, 0x0f, 0xd7, 0xeb, 0xa2, 0x65, 0x57, 0x5f, 0x4a, 0xc3,
	0x56, 0xb2, 0x60, 0x4e, 0x73, 0x2e, 0x68, 0x13, 0x75, 0x09, 0xa4, 0x3b, 0x8d, 0x7b, 0xb9, 0xda, 0xea, 0xab, 0x4c,
	0xaf, 0x1e, 0x4b, 0x8a, 0x1c, 0xf6, 0x48, 0x08, 0x53, 0xe2, 0xf1, 0xfc, 0xa3, 0x23, 0xdd, 0xf3, 0x75, 0x23, 0x7d,
	0x56, 0x5f, 0xae, 0x78, 0x2b, 0xbe, 0x05, 0x9f, 0x52, 0x5b, 0x2f, 0xf6, 0x73, 0xb8, 0x03, 0x3d, 0x86, 0x8f, 0xfb,
	0xc4, 0x3e, 0xcd, 0x63, 0x2a, 0x39, 0xc2, 0xd8, 0x83, 0xe4, 0xfa, 0xda, 0x94, 0x77, 0xdc, 0x1c, 0x75, 0xbf, 0x4b,
	0x39, 0x7a, 0xe3, 0x08, 0x69, 0x56, 0xa7, 0x71, 0x7a, 0x5b, 0xa1, 0xed, 0x84, 0x9a, 0x08, 0xf5, 0x02};

} // namespace builtin_rom


class TempFile
{
public:
	TempFile() : _path(gen_temp_path()), _stream(_path, std::ios::trunc | std::ios::binary | std::ios::out)
	{
		if (!_stream.is_open())
			throw std::runtime_error("failed to open temporary ('" + _path + "') file");
	}

	~TempFile()
	{
		_stream.close();
		std::remove(_path.c_str());
	}

	inline std::string path() const
	{
		return _path;
	}

	inline std::ostream& stream()
	{
		return _stream;
	}

private:
	std::string _path;
	std::ofstream _stream;

private:
	static std::atomic_uint64_t file_id;

	static std::string gen_temp_path()
	{
		// TODO: generate random name?
		std::stringstream ss;
		ss << "__genesis_tmp_file__";
		ss << '.' << file_id.fetch_add(1);
		ss << ".bin";

		return ss.str();
	}
};

std::atomic_uint64_t TempFile::file_id = 0;


template <class Vectors, class Header, class Body>
class ROMConstructor
{
public:
	ROMConstructor(const Vectors& vectors, const Header& header, const Body& body)
		: _vectors(vectors), _header(header), _body(body)
	{
		dump_rom();
	}

	inline std::string path() const
	{
		return tmp_file.path();
	}

private:
	void dump_rom()
	{
		auto write_array = [this](const auto& arr) {
			auto write = [this](auto val) {
				endian::sys_to_big(val);
				tmp_file.stream().write(reinterpret_cast<char*>(&val), sizeof(val));
			};

			std::for_each(std::cbegin(arr), std::cend(arr), write);
		};

		write_array(_vectors);
		write_array(_header);
		write_array(_body);

		tmp_file.stream().flush();
	}

private:
	const Vectors& _vectors;
	const Header& _header;
	const Body& _body;
	TempFile tmp_file;
};


TEST(ROM, WrongPath)
{
	EXPECT_THROW(genesis::rom("/some/path/rom"), std::runtime_error);
	EXPECT_THROW(genesis::rom("/some/path/rom."), std::runtime_error);
	EXPECT_THROW(genesis::rom("/some/path/rom.txt"), std::runtime_error);
}


TEST(ROM, DataParsing)
{
	ROMConstructor rom(builtin_rom::raw_vectors, builtin_rom::raw_header, builtin_rom::raw_body);
	genesis::rom test_rom(rom.path());

	auto check_arrays = [](const auto& arr1, const auto& arr2) {
		ASSERT_EQ(arr1.size(), arr2.size());

		for (size_t i = 0; i < arr1.size(); ++i)
			ASSERT_EQ(arr1[i], arr2[i]);
	};


	check_arrays(test_rom.vectors(), builtin_rom::raw_vectors);
	check_arrays(test_rom.body(), builtin_rom::raw_body);

	ASSERT_EQ(test_rom.checksum(), test_rom.header().rom_checksum);

	ASSERT_EQ(builtin_rom::header, test_rom.header());
}


template <class Vectors, class Header, class Body>
void check_ill_formatted_rom(const Vectors& vectors, const Header& header, const Body& body)
{
	ROMConstructor tmp_rom(vectors, header, body);
	EXPECT_THROW(genesis::rom{tmp_rom.path()}, std::runtime_error);
}

template <class Array>
auto half_array(const Array& arr)
{
	return std::span(arr.data(), arr.size() / 2);
}


TEST(ROM, CorruptedInput)
{
	using namespace builtin_rom;

	/* empty rom */
	check_ill_formatted_rom(empty_array, empty_array, empty_array);

	/* half vectors */
	check_ill_formatted_rom(half_array(raw_vectors), empty_array, empty_array);

	/* empty header */
	check_ill_formatted_rom(raw_vectors, empty_array, empty_array);

	/* helf header */
	check_ill_formatted_rom(raw_vectors, half_array(raw_header), empty_array);

	/* empty body */
	check_ill_formatted_rom(raw_vectors, raw_header, empty_array);
}